#include "render/target/bloom.hpp"
#include "gpu/texture.hpp"

#include <glm/fwd.hpp>
#include <ranges>

namespace render::target
{
	std::expected<void, util::Error> Bloom::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (this->size == size && !downsample_chain.empty() && !upsample_chain.empty()) return {};

		downsample_chain.clear();
		upsample_chain.clear();

		auto mip_size_count = size / 2u;
		std::vector<glm::u32vec2> mip_sizes;
		for (const auto _ : std::views::iota(0u, downsample_mip_count))
		{
			mip_sizes.push_back(mip_size_count);
			mip_size_count /= 2u;
		}

		auto filter_texture_result = gpu::Texture::create(
			device,
			format.create(size.x, size.y, 1, downsample_mip_count),
			"Bloom filter texture"
		);
		if (!filter_texture_result)
			return filter_texture_result.error().forward("Create bloom filter texture failed");
		filter_texture = std::make_unique<gpu::Texture>(std::move(*filter_texture_result));

		for (const auto mip : std::views::iota(0u, downsample_mip_count))
		{
			const auto create_info = format.create(mip_sizes[mip].x, mip_sizes[mip].y, 1, 1);
			auto downsample_texture =
				gpu::Texture::create(device, create_info, std::format("Bloom downsample mip {}", mip));
			if (!downsample_texture)
				return downsample_texture.error().forward("Create bloom downsample texture failed");
			downsample_chain.emplace_back(std::move(*downsample_texture));
		}

		for (const auto mip : std::views::iota(0u, upsample_mip_count))
		{
			const auto create_info = format.create(mip_sizes[mip].x, mip_sizes[mip].y, 1, 1);
			auto upsample_texture =
				gpu::Texture::create(device, create_info, std::format("Bloom upsample mip {}", mip));
			if (!upsample_texture)
				return upsample_texture.error().forward("Create bloom upsample texture failed");
			upsample_chain.emplace_back(std::move(*upsample_texture));
		}

		this->size = size;

		return {};
	}

	const gpu::Texture& Bloom::get_downsample_chain(size_t mip) const noexcept
	{
		return downsample_chain.at(mip);
	}

	const gpu::Texture& Bloom::get_filter_texture() const noexcept
	{
		assert(filter_texture != nullptr);
		return *filter_texture;
	}

	const gpu::Texture& Bloom::get_upsample_chain(size_t mip) const noexcept
	{
		return upsample_chain.at(mip);
	}
}