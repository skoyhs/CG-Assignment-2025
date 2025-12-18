#include "render/target/ao.hpp"
#include <SDL3/SDL_gpu.h>

namespace render::target
{
	std::expected<void, util::Error> AO::cycle(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (auto result = halfres_ao_texture.resize_and_cycle(device, size / 2u); !result)
			return result.error().forward("Resize AO texture failed");
		return {};
	}
}