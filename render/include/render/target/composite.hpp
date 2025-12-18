#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include "graphics/util/smart-texture.hpp"

namespace render::target
{
	///
	/// @brief Composite Render Target
	/// @details Where the final image is rendered to before performing Anti-aliasing
	///
	struct Composite
	{
		static constexpr gpu::Texture::Format composite_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = {.sampler = true, .color_target = true}
		};

		graphics::Auto_texture composite_texture{composite_format, "Composite Texture"};  // Composite Texture

		// Resize all textures
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}