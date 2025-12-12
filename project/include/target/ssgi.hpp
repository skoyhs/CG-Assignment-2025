#pragma once

#include "gpu/texture.hpp"
#include "graphics/util/smart-texture.hpp"

namespace target
{
	struct SSGI
	{
		static constexpr gpu::Texture::Format ssgi_buffer_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
			.usage = {.sampler = true, .compute_storage_write = true}
		};

		graphics::Auto_texture primary_trace_texture{ssgi_buffer_format, "SSGI Trace Texture"};

		///
		/// @brief Resize SSGI render targets
		///
		/// @param size Swapchain size
		///
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}