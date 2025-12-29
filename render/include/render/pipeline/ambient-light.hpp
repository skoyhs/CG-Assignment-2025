#pragma once

#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"

#include "render/target/ao.hpp"
#include "render/target/gbuffer.hpp"

#include <glm/glm.hpp>

namespace render::pipeline
{
	class Ambient_light
	{
	  public:

		struct Param
		{
			glm::vec3 ambient_intensity;  // in reference luminance
			float ao_strength;
		};

		static std::expected<Ambient_light, util::Error> create(SDL_GPUDevice* device) noexcept;

		void render(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			const target::Gbuffer& gbuffer,
			const target::AO& ao,
			const Param& param
		) const noexcept;

	  private:

		graphics::Fullscreen_pass<false> fullscreen_pass;
		gpu::Sampler sampler_nearest;
		gpu::Sampler sampler_linear;

		Ambient_light(
			graphics::Fullscreen_pass<false> fullscreen_pass,
			gpu::Sampler sampler_nearest,
			gpu::Sampler sampler_linear
		) noexcept :
			fullscreen_pass(std::move(fullscreen_pass)),
			sampler_nearest(std::move(sampler_nearest)),
			sampler_linear(std::move(sampler_linear))
		{}

	  public:

		Ambient_light(const Ambient_light&) = delete;
		Ambient_light(Ambient_light&&) = default;
		Ambient_light& operator=(const Ambient_light&) = delete;
		Ambient_light& operator=(Ambient_light&&) = default;
	};
}