#pragma once

#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"

#include "render/target/ao.hpp"
#include "render/target/gbuffer.hpp"

#include <glm/glm.hpp>

namespace render::pipeline
{
	class AO
	{
	  public:

		struct Params
		{
			glm::mat4 camera_mat_inv;
			glm::mat4 view_mat;
			glm::mat4 proj_mat_inv;
			glm::mat4 prev_camera_mat;
			float radius;
			float blend_alpha;
		};

		static std::expected<AO, util::Error> create(SDL_GPUDevice* device) noexcept;

		void render(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			const target::AO& ao_target,
			const target::Gbuffer& gbuffer,
			const Params& params
		) const noexcept;

	  private:

		struct Uniform_params
		{
			alignas(64) glm::mat4 camera_mat_inv;
			alignas(64) glm::mat4 view_mat;
			alignas(64) glm::mat4 proj_mat_inv;
			alignas(64) glm::mat4 prev_camera_mat;
			alignas(4) float random_seed;
			alignas(4) float radius;
			alignas(4) float blend_alpha;

			static Uniform_params from(const Params& params) noexcept;
		};

		gpu::Sampler sampler_linear;
		gpu::Sampler sampler_nearest;
		graphics::Fullscreen_pass<false> fullscreen_pass;

		AO(gpu::Sampler sampler_linear,
		   gpu::Sampler sampler_nearest,
		   graphics::Fullscreen_pass<false> fullscreen_pass) noexcept :
			sampler_linear(std::move(sampler_linear)),
			sampler_nearest(std::move(sampler_nearest)),
			fullscreen_pass(std::move(fullscreen_pass))
		{}

	  public:

		AO(const AO&) = delete;
		AO(AO&&) = default;
		AO& operator=(const AO&) = delete;
		AO& operator=(AO&&) = default;
	};
}