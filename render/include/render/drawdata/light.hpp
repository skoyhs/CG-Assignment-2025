#pragma once

#include "gltf/light.hpp"
#include "render/light-volume.hpp"

#include <SDL3/SDL_gpu.h>
#include <memory>

namespace render::drawdata
{
	struct Light
	{
		glm::mat4 volume_transform;
		glm::mat4 node_transform;

		gltf::Light light;

		std::shared_ptr<const Light_volume> volume;

		static Light from(
			const glm::mat4& node_transform,
			const glm::mat4& volume_transform,
			const gltf::Light& light,
			std::shared_ptr<const Light_volume> volume
		) noexcept;
	};
}