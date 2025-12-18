#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include "graphics/util/smart-texture.hpp"

namespace render::target
{
	struct AO
	{
		static constexpr gpu::Texture::Format ao_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R16_UNORM,
			.usage = {.sampler = true, .color_target = true}
		};

		graphics::Cycle_texture halfres_ao_texture{ao_format, "AO Texture"};  // Ambient Occlusion Texture

		// Resize all textures
		std::expected<void, util::Error> cycle(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}