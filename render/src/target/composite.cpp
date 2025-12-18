#include "render/target/composite.hpp"

namespace render::target
{
	std::expected<void, util::Error> Composite::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		auto result = composite_texture.resize(device, size);
		if (!result) return result.error().forward("Resize composite texture failed");

		return {};
	}
}