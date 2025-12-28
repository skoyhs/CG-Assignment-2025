#pragma once

#include "gltf/model.hpp"
#include "logic/camera-control.hpp"
#include "logic/light-source.hpp"
#include "render/drawdata/light.hpp"
#include "render/light-volume.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

class Logic
{
	/* Camera */

	logic::Camera camera_control;

	/* Lighting Control */

	float light_azimuth = glm::radians(103.0);
	float light_pitch = glm::radians(53.0);

	glm::vec3 light_color = {1.0, 1.0, 1.0};
	float light_intensity = 80000.0;
	float bloom_attenuation = 1.2f;
	float bloom_strength = 0.05f;
	bool use_bloom_mask = true;
	bool show_ceiling = true;

	uint32_t ceiling_node_index = 0;

	float csm_linear_blend = 0.56f;

	void light_control_ui() noexcept;

	/* Antialiasing */

	render::Antialias_mode aa_mode = render::Antialias_mode::MLAA;

	void antialias_control_ui() noexcept;

	/* Statistics */

	void statistic_display_ui() const noexcept;

	/* Animations */

	const float max_door_time = 53.0f / 24.0f;  // seconds
	const float max_door5_time = 5.0;
	const float max_curtain_time = 72.0f / 24.0f;  // seconds

	float door1_position = 0.0f;
	float door2_position = 0.0f;
	float door3_position = 0.0f;
	float door4_position = 0.0f;
	float door5_position = 0.0f;

	float curtain_left_position = 0.0f;
	float curtain_right_position = 0.0f;

	uint32_t door1_node_index = 0;
	uint32_t door2_node_index = 0;
	uint32_t door3_node_index = 0;
	uint32_t door4_node_index = 0;
	uint32_t door5_node_index = 0;

	void animation_control_ui() noexcept;

	/* Lights */

	std::map<std::string, logic::Light_group> light_groups;
	void light_source_control_ui() noexcept;

	Logic() = default;

  public:

	Logic(const Logic&) = delete;
	Logic(Logic&&) = default;
	Logic& operator=(const Logic&) = delete;
	Logic& operator=(Logic&&) = delete;

	static std::expected<Logic, util::Error> create(SDL_GPUDevice* device, const gltf::Model& model) noexcept;

	std::tuple<render::Params, std::vector<gltf::Drawdata>, std::vector<render::drawdata::Light>> logic(
		const backend::SDL_context& context,
		const gltf::Model& model
	) noexcept;
};