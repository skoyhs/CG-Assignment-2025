#include "gltf/light.hpp"

namespace gltf
{
	std::expected<Light, util::Error> parse_light(const tinygltf::Light& light) noexcept
	{
		const auto emission =
			glm::vec3(light.color[0], light.color[1], light.color[2]) * float(light.intensity);

		if (light.type == "directional")
			return Light{
				.type = Light::Type::Directional,
				.emission = emission,
			};

		if (light.type == "spot")
			return Light{
				.type = Light::Type::Spot,
				.emission = emission,
				.range = float(light.range),
				.inner_cone_angle = float(light.spot.innerConeAngle),
				.outer_cone_angle = float(light.spot.outerConeAngle),
			};

		if (light.type == "point")
			return Light{
				.type = Light::Type::Point,
				.emission = emission,
				.range = float(light.range),
			};

		return util::Error(std::format("Unknown light type: {}", light.type));
	}
}