#pragma once

#include "util/error.hpp"

#include <expected>
#include <glm/glm.hpp>
#include <tiny_gltf.h>

namespace gltf
{
	struct Light
	{
		enum class Type
		{
			Spot,
			Point,
			Directional
		};

		Type type;
		glm::vec3 emission;
		float range = 0.0f;             // Spot & Point light only property, ignored for other types
		float inner_cone_angle = 0.0f;  // Spot light only property, ignored for other types
		float outer_cone_angle = 0.0f;  // Spot light only property, ignored for other types
	};

	std::expected<Light, util::Error> parse_light(const tinygltf::Light& light) noexcept;
}