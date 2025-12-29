#include "render/drawdata/light.hpp"

namespace render::drawdata
{
	Light Light::from(
		const glm::mat4& node_transform,
		const glm::mat4& volume_transform,
		const gltf::Light& light,
		std::shared_ptr<const Light_volume> volume
	) noexcept
	{
		return {
			.volume_transform = volume_transform,
			.node_transform = node_transform,
			.light = light,
			.volume = std::move(volume)
		};
	}
}