#include "logic/light-source.hpp"

#include "asset/light-volume.hpp"
#include "render/light-volume.hpp"
#include "util/asset.hpp"
#include "zip/zip.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

namespace logic
{
	static std::expected<nlohmann::json, util::Error> load_json() noexcept
	{
		const auto json_text =
			util::get_asset(resource_asset::light_volume, "light-volume-table.json")
				.and_then(zip::Decompress())
				.transform([](std::vector<std::byte> data) {
					return std::string(reinterpret_cast<const char*>(data.data()), data.size());
				});
		if (!json_text) return json_text.error().forward("Can't find light volume table config");

		try
		{
			nlohmann::json json = nlohmann::json::parse(*json_text);
			if (!json.is_object()) return util::Error("Light group JSON is not an object");
			return json;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			return util::Error(std::format("Parse light group JSON failed: {}", e.what()));
		}
	}

	static std::expected<std::map<std::string, Light_group>, util::Error> get_groups(
		const nlohmann::json& json,
		const gltf::Model& model
	) noexcept
	{
		try
		{
			if (!json.is_array()) return util::Error("JSON root is not an array");
			std::map<std::string, Light_group> groups;

			for (const auto& [key, value] : json.items())
			{
				if (!value.is_object())
					return util::Error(std::format("Light group index {} is not an object", key));
				if (!value.contains("name"))
					return util::Error(std::format("Light group index {} has no name", key));
				if (!value.contains("display"))
					return util::Error(std::format("Light group index {} has no display name", key));

				const auto group_name = value["name"].get<std::string>();

				std::vector<uint32_t> emission_nodes;
				if (value.contains("emission_nodes"))
				{
					const auto nodes = value["emission_nodes"];
					if (!nodes.is_array())
						return util::Error(
							std::format("Light group '{}' emission_nodes is not an array", group_name)
						);

					for (const auto& [_, node_name] : nodes.items())
					{
						if (!node_name.is_string())
							return util::Error(
								std::format(
									"Light group '{}' emission_nodes has non-string entry",
									group_name
								)
							);
						const auto find_node_result = model.find_node_by_name(node_name.get<std::string>());
						if (!find_node_result)
							return util::Error(
								std::format(
									"Can't find emission node '{}' for light group '{}'",
									node_name.get<std::string>(),
									group_name
								)
							);
						emission_nodes.push_back(*find_node_result);
					}
				}

				auto group = Light_group{
					.display_name = value["display"].get<std::string>(),
					.lights = {},
					.emission_nodes = std::move(emission_nodes),
					.enabled = true,
				};

				groups.emplace(value["name"].get<std::string>(), group);
			}

			return groups;
		}
		catch (const nlohmann::json::type_error& e)
		{
			return util::Error(std::format("Parse light group JSON failed: {}", e.what()));
		}
	}

	static std::expected<void, util::Error> assign_lights_to_groups(
		SDL_GPUDevice* device,
		std::map<std::string, Light_group>& groups,
		const nlohmann::json& json,
		const gltf::Model& model
	) noexcept
	{
		if (!json.is_array()) return util::Error("JSON root is not an array");

		for (const auto& [idx_str, value] : json.items())
		{
			if (!value.is_object())
				return util::Error(std::format("Light group index {} is not an object", idx_str));
			if (!value.contains("node_name") || !value.contains("group") || !value.contains("path"))
				return util::Error(std::format("Light group index {} is missing required fields", idx_str));

			const auto node_name = value["node_name"].get<std::string>();
			const auto group_name = value["group"].get<std::string>();
			const auto path = value["path"].get<std::string>();

			const auto find_light_result = model.find_light_by_name(node_name);
			if (!find_light_result)
				return util::Error(std::format("Can't find light '{}' in model", node_name));

			const auto find_group_it = groups.find(group_name);
			if (find_group_it == groups.end())
				return util::Error(std::format("Can't find light group '{}'", group_name));

			auto volume =
				util::get_asset(resource_asset::light_volume, path)
					.and_then(zip::Decompress())
					.and_then(wavefront::parse_raw)
					.and_then(
						[device, &node_name](const wavefront::Object& raw_model)
							-> std::expected<render::Light_volume, util::Error> {
							return render::Light_volume::from_model(
								device,
								raw_model,
								std::format("Light volume '{}'", node_name)
							);
						}
					);
			if (!volume)
				return volume.error().forward(std::format("Load light volume '{}' failed", node_name));

			const auto [node_index, light] = *find_light_result;
			find_group_it->second.lights.push_back(
				Light_source{
					.volume =
						std::make_shared<render::Light_volume>(std::move(*volume)),  // to be filled later
					.light = light,
					.node_index = node_index
				}
			);
		}

		return {};
	}

	std::expected<std::map<std::string, Light_group>, util::Error> load_light_groups(
		SDL_GPUDevice* device,
		const gltf::Model& model
	) noexcept
	{
		auto json_result = load_json();
		if (!json_result) return json_result.error().forward("Load light group JSON failed");
		auto json = std::move(*json_result);

		if (!json.contains("groups")) return util::Error("Light group JSON has no 'groups' field");
		auto groups_result = get_groups(json["groups"], model);
		if (!groups_result) return groups_result.error().forward("Parse light groups failed");
		auto groups = std::move(*groups_result);

		if (!json.contains("lights")) return util::Error("Light group JSON has no 'lights' field");
		const auto assign_result = assign_lights_to_groups(device, groups, json["lights"], model);
		if (!assign_result) return assign_result.error().forward("Assign lights to groups failed");

		return groups;
	}
}