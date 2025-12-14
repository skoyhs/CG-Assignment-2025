#include "pipeline/ssgi.hpp"
#include "asset/graphic-asset.hpp"
#include "asset/shader/init-temporal.comp.hpp"
#include "gpu/compute-pass.hpp"
#include "gpu/compute-pipeline.hpp"
#include "graphics/util/quick-create.hpp"
#include "image/io.hpp"
#include "util/as-byte.hpp"
#include "util/asset.hpp"
#include "zip/zip.hpp"

#include <SDL3/SDL_gpu.h>
#include <random>

namespace pipeline
{
	SSGI::Initial_sample_param SSGI::Initial_sample_param::from_param(
		const Param& param,
		const glm::uvec2& resolution
	) noexcept
	{
		static thread_local std::mt19937 generator{std::random_device{}()};
		static thread_local std::uniform_int_distribution<int> distribution{0, 128};

		const glm::mat4 inv_proj_mat = glm::inverse(param.proj_mat);

		const glm::vec4 near_plane_center_clip = {0.0, 0.0, 1.0, 1.0};
		const glm::vec4 near_plane_corner_clip = {1.0, 1.0, 1.0, 1.0};
		glm::vec4 near_plane_center_view = inv_proj_mat * near_plane_center_clip;
		glm::vec4 near_plane_corner_view = inv_proj_mat * near_plane_corner_clip;
		near_plane_center_view /= near_plane_center_view.w;
		near_plane_corner_view /= near_plane_corner_view.w;

		const glm::vec2 near_plane_span = {
			near_plane_corner_view.x - near_plane_center_view.x,
			near_plane_corner_view.y - near_plane_center_view.y
		};

		const glm::ivec2 time_noise = {distribution(generator), distribution(generator)};

		return Initial_sample_param{
			.inv_proj_mat = inv_proj_mat,
			.proj_mat = param.proj_mat,
			.inv_view_mat = glm::inverse(param.view_mat),
			.view_mat = param.view_mat,
			.back_proj_mat = param.prev_view_proj_mat * glm::inverse(param.view_mat),
			.inv_proj_mat_col3 = inv_proj_mat[2],
			.inv_proj_mat_col4 = inv_proj_mat[3],
			.resolution = resolution,
			.time_noise = time_noise,
			.near_plane_span = near_plane_span,
			.near_plane = -near_plane_center_view.z,
			.max_scene_distance = param.max_scene_distance,
			.distance_attenuation = param.distance_attenuation
		};
	}

	std::expected<SSGI, util::Error> SSGI::create(SDL_GPUDevice* device) noexcept
	{
		/* Read Image */

		const auto noise_image_name = "blue-noise.png";
		auto noise_texture =
			util::get_asset(resource_asset::graphic_asset, noise_image_name)
				.and_then(zip::Decompress())
				.and_then(image::load_from_memory<image::Precision::U16, image::Format::RGBA>)
				.and_then([device](const auto& image) {
					return graphics::create_texture_from_image(
						device,
						gpu::Texture::Format{
							.type = SDL_GPU_TEXTURETYPE_2D,
							.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM,
							.usage = {.sampler = true}
						},
						image,
						"SSGI Blue Noise Texture"
					);
				});
		if (!noise_texture) return noise_texture.error().forward("Create SSGI blue noise texture failed");

		/* Create Samplers */

		const auto linear_sampler_create_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Linear,
			.mag_filter = gpu::Sampler::Filter::Linear,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Repeat,
			.address_mode_v = gpu::Sampler::Address_mode::Repeat,
			.address_mode_w = gpu::Sampler::Address_mode::Repeat
		};

		const auto nearest_sampler_create_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_w = gpu::Sampler::Address_mode::Clamp_to_edge,
			.min_lod = 0,
			.max_lod = 10
		};

		const auto noise_sampler_create_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Repeat,
			.address_mode_v = gpu::Sampler::Address_mode::Repeat,
			.address_mode_w = gpu::Sampler::Address_mode::Repeat
		};

		auto nearest_sampler = gpu::Sampler::create(device, nearest_sampler_create_info);
		auto noise_sampler = gpu::Sampler::create(device, noise_sampler_create_info);
		auto linear_sampler = gpu::Sampler::create(device, linear_sampler_create_info);
		if (!nearest_sampler) return nearest_sampler.error().forward("Create SSGI nearest sampler failed");
		if (!noise_sampler) return noise_sampler.error().forward("Create SSGI noise sampler failed");
		if (!linear_sampler) return linear_sampler.error().forward("Create SSGI linear sampler failed");

		/* Create Pipeline */

		const gpu::Compute_pipeline::Create_info pipeline_create_info{
			.shader_data = shader_asset::init_temporal_comp,
			.num_samplers = 9,
			.num_readwrite_storage_textures = 4,
			.num_uniform_buffers = 1,
			.threadcount_x = 8,
			.threadcount_y = 8,
			.threadcount_z = 1
		};

		auto ssgi_pipeline =
			gpu::Compute_pipeline::create(device, pipeline_create_info, "SSGI Trace Pipeline");
		if (!ssgi_pipeline) return ssgi_pipeline.error().forward("Create SSGI pipeline failed");

		return SSGI(
			std::move(*ssgi_pipeline),
			std::move(*noise_sampler),
			std::move(*nearest_sampler),
			std::move(*linear_sampler),
			std::move(*noise_texture)
		);
	}

	std::expected<void, util::Error> SSGI::render(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{

		const auto initial_sample_result =
			this->run_initial_sample(command_buffer, light_buffer, gbuffer, ssgi_target, param, resolution);
		if (!initial_sample_result)
			return initial_sample_result.error().forward("Run SSGI initial sample failed");

		return {};
	}

	std::expected<void, util::Error> SSGI::run_initial_sample(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{
		const auto half_res = (resolution + 1u) / 2u;
		const auto dispatch_size = (half_res + 7u) / 8u;

		const Initial_sample_param initial_sample_param = Initial_sample_param::from_param(param, resolution);
		command_buffer.push_uniform_to_compute(0, util::as_bytes(initial_sample_param));

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture1{
			.texture = ssgi_target.temporal_reservoir_texture1.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture2{
			.texture = ssgi_target.temporal_reservoir_texture2.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture3{
			.texture = ssgi_target.temporal_reservoir_texture3.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture4{
			.texture = ssgi_target.temporal_reservoir_texture4.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const std::array write_bindings =
			{write_binding_texture1, write_binding_texture2, write_binding_texture3, write_binding_texture4};

		command_buffer.push_debug_group("SSGI Pass");
		auto result = command_buffer.run_compute_pass(
			write_bindings,
			{},
			[this, &light_buffer, &gbuffer, &ssgi_target, dispatch_size](
				const gpu::Compute_pass& compute_pass
			) {
				compute_pass.bind_pipeline(ssgi_pipeline);
				compute_pass.bind_samplers(
					0,
					light_buffer.light_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.lighting_info_texture->bind_with_sampler(nearest_sampler),
					noise_texture.bind_with_sampler(noise_sampler),
					gbuffer.depth_value_texture.prev().bind_with_sampler(linear_sampler),

					ssgi_target.temporal_reservoir_texture1.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture2.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture3.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture4.prev().bind_with_sampler(nearest_sampler)
				);
				compute_pass.dispatch(dispatch_size.x, dispatch_size.y, 1);
			}
		);
		command_buffer.pop_debug_group();

		return std::move(result).transform_error(util::Error::forward_fn("SSGI pass failed"));
	}
}