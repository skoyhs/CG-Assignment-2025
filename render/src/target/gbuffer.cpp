#include "render/target/gbuffer.hpp"
#include <SDL3/SDL_gpu.h>

namespace render::target
{
	std::expected<void, util::Error> Gbuffer::cycle(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (auto result = depth_texture.resize(device, size); !result) return result.error();
		if (auto result = depth_value_texture.resize_and_cycle(device, size); !result) return result.error();
		if (auto result = albedo_texture.resize(device, size); !result) return result.error();
		if (auto result = lighting_info_texture.resize(device, size); !result) return result.error();
		return {};
	}
}
