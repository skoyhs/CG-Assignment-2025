#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "gpu/texture.hpp"

namespace render::target
{
	class Bloom
	{
	  public:

		static constexpr gpu::Texture::Format format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
			.usage = {
					  .sampler = true,
					  .color_target = true,
					  .compute_storage_read = true,
					  .compute_storage_write = true,
					  }
		};

		static constexpr size_t downsample_mip_count = 9;
		static constexpr size_t upsample_mip_count = downsample_mip_count - 1;

		Bloom() = default;

		///
		/// @brief Resize bloom textures
		///
		/// @param size Top level texture size
		/// @return Resize results
		///
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;

		///
		/// @brief Get downsample chain
		///
		/// @param mip_level Mipmap level
		/// @return Downscale chain
		///
		const gpu::Texture& get_downsample_chain(size_t mip_level) const noexcept;

		///
		/// @brief Get filter texture
		///
		/// @return Filter texture
		///
		const gpu::Texture& get_filter_texture() const noexcept;

		///
		/// @brief Get upsample chain
		///
		/// @param mip_level Mipmap level
		/// @return Upscale chain
		///
		const gpu::Texture& get_upsample_chain(size_t mip_level) const noexcept;

	  private:

		glm::u32vec2 size = {0, 0};
		std::unique_ptr<gpu::Texture> filter_texture;
		std::vector<gpu::Texture> downsample_chain, upsample_chain;

	  public:

		Bloom(const Bloom&) = delete;
		Bloom(Bloom&&) = default;
		Bloom& operator=(const Bloom&) = delete;
		Bloom& operator=(Bloom&&) = default;

		~Bloom() = default;
	};
}