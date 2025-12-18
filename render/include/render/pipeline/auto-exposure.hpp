#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/compute-pipeline.hpp"
#include "gpu/sampler.hpp"
#include "gpu/texture.hpp"

#include "render/target/auto-exposure.hpp"
#include "render/target/light.hpp"

#include <expected>
#include <glm/glm.hpp>

namespace render::pipeline
{
	class Auto_exposure
	{
	  public:

		struct Params
		{
			float min_luminance;
			float max_luminance;
			float eye_adaptation_rate;
			float delta_time;
		};

		static std::expected<Auto_exposure, util::Error> create(SDL_GPUDevice* device) noexcept;

		std::expected<void, util::Error> compute(
			const gpu::Command_buffer& command_buffer,
			const target::Auto_exposure& target,
			const target::Light_buffer& light_buffer,
			const Params& params,
			glm::u32vec2 size
		) const noexcept;

	  private:

		struct Internal_param_histogram
		{
			float min_histogram_value;
			float log_min_histogram_value;  // = log2(min)
			float dynamic_range;            // = log2(max) - log2(min)
			alignas(8) glm::uvec2 input_size;

			static Internal_param_histogram from(const Params& params, glm::u32vec2 size) noexcept;
		};

		struct Internal_param_avg
		{
			float min_histogram_value;
			float log_min_histogram_value;  // = log2(min)
			float dynamic_range;            // = log2(max) - log2(min)
			float adaptation_rate;

			static Internal_param_avg from(const Params& params) noexcept;
		};

		gpu::Compute_pipeline clear_pipeline;
		gpu::Compute_pipeline histogram_pipeline;
		gpu::Compute_pipeline avg_pipeline;
		gpu::Texture mask_texture;
		gpu::Sampler mask_sampler;

		Auto_exposure(
			gpu::Compute_pipeline clear_pipeline,
			gpu::Compute_pipeline histogram_pipeline,
			gpu::Compute_pipeline avg_pipeline,
			gpu::Texture mask_texture,
			gpu::Sampler mask_sampler
		) :
			clear_pipeline(std::move(clear_pipeline)),
			histogram_pipeline(std::move(histogram_pipeline)),
			avg_pipeline(std::move(avg_pipeline)),
			mask_texture(std::move(mask_texture)),
			mask_sampler(std::move(mask_sampler))
		{}

	  public:

		Auto_exposure(const Auto_exposure&) = delete;
		Auto_exposure(Auto_exposure&&) = default;
		Auto_exposure& operator=(const Auto_exposure&) = delete;
		Auto_exposure& operator=(Auto_exposure&&) = default;
	};
}