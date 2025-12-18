#include "render/target/shadow.hpp"

namespace render::target
{
	std::expected<void, util::Error> Shadow::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (auto result = depth_texture_level0.resize(device, size); !result)
			return result.error().forward("Resize Shadow depth texture failed");

		if (auto result = depth_texture_level1.resize(device, size * 3u / 4u); !result)
			return result.error().forward("Resize Shadow depth texture level 1 failed");

		if (auto result = depth_texture_level2.resize(device, size / 2u); !result)
			return result.error().forward("Resize Shadow depth texture level 2 failed");

		return {};
	}
}