#pragma once

#include <glm/glm.hpp>

#include "graphics/util/smart-texture.hpp"

namespace target
{
	///
	/// @brief Physical Light Render Target
	/// @details Stores the accumulated light information from all light sources.
	/// #### Light Buffer Texture
	/// Format: `RGBA@FLOAT16`
	/// - `R`: Red Channel
	/// - `G`: Green Channel
	/// - `B`: Blue Channel
	/// - `A`: Unused
	///
	struct Light_buffer
	{
		// Light Buffer Texture Format
		static constexpr gpu::Texture::Format light_buffer_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
			.usage = {.sampler = true, .color_target = true, .compute_storage_read = true}
		};

		graphics::Cycle_texture light_texture{
			light_buffer_format,
			"Physical Light Texture"
		};  // Light Buffer Texture

		// Resize the render target
		std::expected<void, util::Error> cycle(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}