#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/sampler.hpp"
#include "gpu/texture.hpp"
#include "graphics/util/fullscreen-pass.hpp"
#include "util/error.hpp"

#include <SDL3/SDL_gpu.h>
#include <expected>

namespace graphics
{
	///
	/// @brief Texture copier using a render pass, for copying from depth to color
	///
	///
	class Renderpass_copy
	{
	  public:

		static std::expected<Renderpass_copy, util::Error> create(
			SDL_GPUDevice* device,
			size_t channels,
			gpu::Texture::Format dst_format
		) noexcept;

		///
		/// @brief Copy texture data from src to dst
		/// @note This function only copies to the first mipmap level
		/// @param command_buffer Command buffer
		/// @param src Source texture
		/// @param dst Destination texture
		/// @return Result
		///
		std::expected<void, util::Error> copy(
			const gpu::Command_buffer& command_buffer,
			SDL_GPUTexture* src,
			SDL_GPUTexture* dst
		) const noexcept;

	  private:

		Fullscreen_pass<true> copy_pass;
		gpu::Sampler sampler;

		Renderpass_copy(Fullscreen_pass<true> copy_pass, gpu::Sampler sampler) :
			copy_pass(std::move(copy_pass)),
			sampler(std::move(sampler))
		{}

	  public:

		Renderpass_copy(const Renderpass_copy&) = delete;
		Renderpass_copy(Renderpass_copy&&) = default;
		Renderpass_copy& operator=(const Renderpass_copy&) = delete;
		Renderpass_copy& operator=(Renderpass_copy&&) = default;
	};
}