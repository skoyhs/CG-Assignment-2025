#pragma once

#include "gltf/light.hpp"
#include "gltf/model.hpp"
#include "render/light-volume.hpp"

#include <cstdint>
#include <memory>

namespace logic
{
	struct Light_source
	{
		std::shared_ptr<const render::Light_volume> volume;
		gltf::Light light;
		uint32_t node_index;
	};

	struct Light_group
	{
		std::string display_name;
		std::vector<Light_source> lights;
		std::vector<uint32_t> emission_nodes;
		bool enabled = true;
	};

	std::expected<std::map<std::string, Light_group>, util::Error> load_light_groups(
		SDL_GPUDevice* device,
		const gltf::Model& model
	) noexcept;
}