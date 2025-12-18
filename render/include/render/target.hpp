#pragma once

#include "render/target/ao.hpp"
#include "render/target/auto-exposure.hpp"
#include "render/target/bloom.hpp"
#include "render/target/composite.hpp"
#include "render/target/gbuffer.hpp"
#include "render/target/light.hpp"
#include "render/target/shadow.hpp"
#include "render/target/ssgi.hpp"

namespace render
{
	struct Target
	{
		target::Gbuffer gbuffer_target;
		target::Shadow shadow_target;
		target::Light_buffer light_buffer_target;
		target::Composite composite_target;
		target::AO ao_target;
		target::Auto_exposure auto_exposure_target;
		target::Bloom bloom_target;
		target::SSGI ssgi_target;

		static std::expected<Target, util::Error> create(SDL_GPUDevice* device) noexcept;

		std::expected<void, util::Error> resize_or_cycle(
			SDL_GPUDevice* device,
			glm::u32vec2 swapchain_size
		) noexcept;
	};
}