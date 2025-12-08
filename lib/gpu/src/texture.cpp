#include "gpu/texture.hpp"
#include "gpu/util.hpp"
#include <SDL3/SDL_gpu.h>

namespace gpu
{
	std::expected<Texture, util::Error> Texture::create(
		SDL_GPUDevice* device,
		const SDL_GPUTextureCreateInfo& create_info,
		const std::string& name
	) noexcept
	{
		assert(device != nullptr);

		if (create_info.width == 0
			|| create_info.height == 0
			|| create_info.layer_count_or_depth == 0
			|| create_info.num_levels == 0)
			return util::Error("Texture dimensions and mip levels must be greater than zero");

		auto* const texture = SDL_CreateGPUTexture(device, &create_info);
		if (texture == nullptr) RETURN_SDL_ERROR;

		SDL_SetGPUTextureName(device, texture, name.c_str());

		return Texture(device, texture);
	}

	Texture::Usage::operator SDL_GPUTextureUsageFlags(this Usage self) noexcept
	{
		return std::bit_cast<uint8_t>(self) & 0x7f;
	}

	SDL_GPUTextureCreateInfo Texture::Format::create(
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t mip_levels,
		SDL_GPUSampleCount sample_count
	) const noexcept
	{
		return SDL_GPUTextureCreateInfo{
			.type = type,
			.format = format,
			.usage = usage,
			.width = width,
			.height = height,
			.layer_count_or_depth = depth,
			.num_levels = mip_levels,
			.sample_count = sample_count,
			.props = 0
		};
	}

	bool Texture::Format::supported_on(SDL_GPUDevice* device) const noexcept
	{
		assert(device != nullptr);

		return SDL_GPUTextureSupportsFormat(device, format, type, usage);
	}

	SDL_GPUTextureSamplerBinding Texture::bind_with_sampler(SDL_GPUSampler* sampler) const noexcept
	{
		return {.texture = *this, .sampler = sampler};
	}
}