#include "render/pipeline/auto-exposure.hpp"

#include "asset/shader/avg.comp.hpp"
#include "asset/shader/clear.comp.hpp"
#include "asset/shader/histogram.comp.hpp"
#include "gpu/compute-pipeline.hpp"
#include "gpu/sampler.hpp"
#include "graphics/util/quick-create.hpp"
#include "image/io.hpp"
#include "util/as-byte.hpp"
#include "util/asset.hpp"

#include "asset/graphic-asset.hpp"
#include "zip/zip.hpp"

#include <SDL3/SDL_gpu.h>

namespace render::pipeline
{
	std::expected<Auto_exposure, util::Error> Auto_exposure::create(SDL_GPUDevice* device) noexcept
	{
		const auto clear_pipeline_info = gpu::Compute_pipeline::Create_info{
			.shader_data = shader_asset::clear_comp,
			.num_readwrite_storage_buffers = 1,
			.threadcount_x = 16,
			.threadcount_y = 16,
			.threadcount_z = 1
		};

		const auto histogram_pipeline_info = gpu::Compute_pipeline::Create_info{
			.shader_data = shader_asset::histogram_comp,
			.num_samplers = 1,
			.num_readonly_storage_textures = 1,
			.num_readwrite_storage_buffers = 1,
			.num_uniform_buffers = 1,
			.threadcount_x = 16,
			.threadcount_y = 16,
			.threadcount_z = 1
		};

		const auto avg_pipeline_info = gpu::Compute_pipeline::Create_info{
			.shader_data = shader_asset::avg_comp,
			.num_readonly_storage_buffers = 2,
			.num_readwrite_storage_buffers = 1,
			.num_uniform_buffers = 1,
			.threadcount_x = 1,
			.threadcount_y = 1,
			.threadcount_z = 1
		};

		auto clear_pipeline_result =
			gpu::Compute_pipeline::create(device, clear_pipeline_info, "Auto-exposure Clear Pipeline");
		if (!clear_pipeline_result)
			return clear_pipeline_result.error().forward("Create clear pipeline failed");

		auto histogram_pipeline_result = gpu::Compute_pipeline::create(
			device,
			histogram_pipeline_info,
			"Auto-exposure Histogram Pipeline"
		);
		if (!histogram_pipeline_result)
			return histogram_pipeline_result.error().forward("Create histogram pipeline failed");

		auto avg_pipeline_result =
			gpu::Compute_pipeline::create(device, avg_pipeline_info, "Auto-exposure Average Pipeline");
		if (!avg_pipeline_result)
			return avg_pipeline_result.error().forward("Create average luminance pipeline failed");

		auto mask_texture =
			util::get_asset(resource_asset::graphic_asset, "exposure-mask.png")
				.and_then(zip::Decompress())
				.and_then(image::load_from_memory<image::Precision::U8, image::Format::Luminance>)
				.and_then([device](const image::Image<image::Precision::U8, image::Format::Luminance>& img) {
					return graphics::create_texture_from_image(
						device,
						gpu::Texture::Format{
							.type = SDL_GPU_TEXTURETYPE_2D,
							.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM,
							.usage = {.sampler = true}
						},
						img,
						"Auto exposure mask texture"
					);
				});
		if (!mask_texture) return mask_texture.error().forward("Create auto exposure mask texture failed");

		auto mask_sampler = gpu::Sampler::create(
			device,
			gpu::Sampler::Create_info{
				.min_filter = gpu::Sampler::Filter::Linear,
				.mag_filter = gpu::Sampler::Filter::Linear,
				.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
				.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge
			}
		);
		if (!mask_sampler) return mask_sampler.error().forward("Create auto exposure mask sampler failed");

		return Auto_exposure(
			std::move(*clear_pipeline_result),
			std::move(*histogram_pipeline_result),
			std::move(*avg_pipeline_result),
			std::move(*mask_texture),
			std::move(*mask_sampler)
		);
	}

	std::expected<void, util::Error> Auto_exposure::compute(
		const gpu::Command_buffer& command_buffer,
		const target::Auto_exposure& target,
		const target::Light_buffer& light_buffer,
		const Params& params,
		glm::u32vec2 size
	) const noexcept
	{
		const auto [bin_buffer, prev_result_buffer, result_buffer] = target.get_current_frame();

		const auto bin_buffer_binding = SDL_GPUStorageBufferReadWriteBinding{
			.buffer = bin_buffer,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const auto bin_buffer_binding_not_cycle = SDL_GPUStorageBufferReadWriteBinding{
			.buffer = bin_buffer,
			.cycle = false,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const auto result_buffer_binding = SDL_GPUStorageBufferReadWriteBinding{
			.buffer = result_buffer,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		command_buffer.push_debug_group("Auto Exposure");
		{
			command_buffer.push_debug_group("Clear Histogram Bins");
			{
				const auto clear_pass_result = command_buffer.run_compute_pass(
					{},
					std::to_array({bin_buffer_binding}),
					[this](const gpu::Compute_pass& compute_pass) {
						compute_pass.bind_pipeline(clear_pipeline);
						compute_pass.dispatch(1, 1, 1);
					}
				);
				if (!clear_pass_result)
					return clear_pass_result.error().forward("Auto exposure clear pass failed");
			}
			command_buffer.pop_debug_group();

			command_buffer.push_debug_group("Compute Histogram");
			{
				const auto histogram_params = Internal_param_histogram::from(params, size);
				command_buffer.push_uniform_to_compute(0, util::as_bytes(histogram_params));

				const auto histogram_pass_result = command_buffer.run_compute_pass(
					{},
					std::to_array({bin_buffer_binding_not_cycle}),
					[this, &light_buffer, size](const gpu::Compute_pass& compute_pass) {
						compute_pass.bind_pipeline(histogram_pipeline);
						compute_pass.bind_samplers(0, mask_texture.bind_with_sampler(mask_sampler));
						compute_pass.bind_storage_textures(0, light_buffer.light_texture.current());
						compute_pass.dispatch((size.x + 15) / 16, (size.y + 15) / 16, 1);
					}
				);
				if (!histogram_pass_result)
					return histogram_pass_result.error().forward("Auto exposure histogram pass failed");
			}
			command_buffer.pop_debug_group();

			command_buffer.push_debug_group("Compute Average Luminance");
			{
				const auto avg_params = Internal_param_avg::from(params);
				command_buffer.push_uniform_to_compute(0, util::as_bytes(avg_params));

				const auto avg_pass_result = command_buffer.run_compute_pass(
					{},
					std::to_array({result_buffer_binding}),
					[this, &bin_buffer, &prev_result_buffer](const gpu::Compute_pass& compute_pass) {
						compute_pass.bind_pipeline(avg_pipeline);
						compute_pass.bind_storage_buffers(0, bin_buffer, prev_result_buffer);
						compute_pass.dispatch(1, 1, 1);
					}
				);
				if (!avg_pass_result)
					return avg_pass_result.error().forward("Auto exposure average pass failed");
			}
			command_buffer.pop_debug_group();
		}
		command_buffer.pop_debug_group();

		return {};
	}

	Auto_exposure::Internal_param_histogram Auto_exposure::Internal_param_histogram::from(
		const Auto_exposure::Params& params,
		glm::u32vec2 size
	) noexcept
	{
		return Internal_param_histogram{
			.min_histogram_value = params.min_luminance,
			.log_min_histogram_value = glm::log2(params.min_luminance),
			.dynamic_range = glm::log2(params.max_luminance) - glm::log2(params.min_luminance),
			.input_size = size
		};
	}

	Auto_exposure::Internal_param_avg Auto_exposure::Internal_param_avg::from(const Params& params) noexcept
	{
		return Internal_param_avg{
			.min_histogram_value = params.min_luminance,
			.log_min_histogram_value = glm::log2(params.min_luminance),
			.dynamic_range = glm::log2(params.max_luminance) - glm::log2(params.min_luminance),
			.adaptation_rate = params.eye_adaptation_rate * params.delta_time,
		};
	}
}