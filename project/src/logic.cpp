#include "logic.hpp"
#include "backend/sdl.hpp"
#include "render/drawdata/light.hpp"
#include "render/param.hpp"

#include <imgui.h>
#include <ranges>

void Logic::light_control_ui() noexcept
{
	ImGui::SeparatorText("光照");

	ImGui::SliderAngle("方位角", &light_azimuth, -180.0, 180.0);
	ImGui::SliderAngle("高度角", &light_pitch, -89.0, 89.0);
	ImGui::ColorEdit3("颜色", &light_color.r);
	ImGui::SliderFloat("强度", &light_intensity, 10.0f, 300000.0f, "%.1f", ImGuiSliderFlags_Logarithmic);

	ImGui::Separator();

	ImGui::SliderFloat("CSM 线性混合", &csm_linear_blend, 0.0f, 1.0f);

	ImGui::Separator();

	ImGui::SliderFloat("Bloom 衰减", &bloom_attenuation, 0.0f, 5.0f);
	ImGui::SliderFloat("Bloom 强度", &bloom_strength, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
	ImGui::Checkbox("脏镜头效果", &use_bloom_mask);

	ImGui::Separator();
	ImGui::Checkbox("显示天花板", &show_ceiling);
}

void Logic::antialias_control_ui() noexcept
{
	using namespace render;

	ImGui::SeparatorText("抗锯齿");

	if (ImGui::RadioButton("无抗锯齿", aa_mode == Antialias_mode::None)) aa_mode = Antialias_mode::None;
	if (ImGui::RadioButton("FXAA", aa_mode == Antialias_mode::FXAA)) aa_mode = Antialias_mode::FXAA;
	if (ImGui::RadioButton("MLAA", aa_mode == Antialias_mode::MLAA)) aa_mode = Antialias_mode::MLAA;
	if (ImGui::RadioButton("SMAA", aa_mode == Antialias_mode::SMAA)) aa_mode = Antialias_mode::SMAA;
}

void Logic::statistic_display_ui() const noexcept
{
	const auto& io = ImGui::GetIO();

	ImGui::SeparatorText("统计");

	ImGui::Text("Res: (%.0f, %.0f)", io.DisplaySize.x, io.DisplaySize.y);
	ImGui::Text("FPS: %.1f FPS", io.Framerate);
}

void Logic::animation_control_ui() noexcept
{
	ImGui::SeparatorText("动画");

	ImGui::SliderFloat("门1", &door1_position, 0.0f, 1.0f);
	ImGui::SliderFloat("门2", &door2_position, 0.0f, 1.0f);
	ImGui::SliderFloat("门3", &door3_position, 0.0f, 1.0f);
	ImGui::SliderFloat("门4", &door4_position, 0.0f, 1.0f);
	ImGui::SliderFloat("门5", &door5_position, 0.0f, 1.0f);

	ImGui::Separator();

	ImGui::SliderFloat("左窗帘", &curtain_left_position, 0.0f, 1.0f);
	ImGui::SliderFloat("右窗帘", &curtain_right_position, 0.0f, 1.0f);
}

void Logic::light_source_control_ui() noexcept
{
	ImGui::SeparatorText("光源");
	for (auto& [_, light] : light_groups)
	{
		ImGui::Checkbox(light.display_name.c_str(), &light.enabled);
	}
}

static void draw_handle_hover(
	const render::Camera_matrices& camera_matrices,
	glm::mat4 handle_matrix,
	size_t index
)
{
	const auto handle_position = glm::vec3(handle_matrix[3]);

	const auto handle_ndc_h =
		camera_matrices.proj_matrix * camera_matrices.view_matrix * glm::vec4(handle_position, 1.0f);
	const auto handle_ndc = handle_ndc_h / handle_ndc_h.w;

	if (handle_ndc_h.w > 0.0f && glm::all(glm::lessThan(glm::abs(glm::vec2(handle_ndc)), glm::vec2(1.0))))
	{
		const auto [width, height] = ImGui::GetIO().DisplaySize;
		const auto handle_uv = glm::fma(glm::vec2(handle_ndc), glm::vec2(0.5, -0.5), glm::vec2(0.5));
		const auto handle_puv = handle_uv * glm::vec2(width, height);

		ImGui::SetNextWindowPos(ImVec2(handle_puv.x, handle_puv.y), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		if (ImGui::Begin(
				std::format("##Handle_{}", index).c_str(),
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar
			))
		{
			ImGui::Text("把手");
			ImGui::Text("(%.2f, %.2f, %.2f)", handle_position.x, handle_position.y, handle_position.z);
		}
		ImGui::End();
	}
}

std::expected<Logic, util::Error> Logic::create(SDL_GPUDevice* device, const gltf::Model& model) noexcept
{
	Logic logic;

	const auto door1_node_index = model.find_node_by_name("Door1-Handle");
	const auto door2_node_index = model.find_node_by_name("Door2-Handle");
	const auto door3_node_index = model.find_node_by_name("Door3-Handle");
	const auto door4_node_index = model.find_node_by_name("Door4-Handle");
	const auto door5_node_index = model.find_node_by_name("Door5-Handle");
	const auto ceiling_node_index = model.find_node_by_name("Ceiling");

	if (!door1_node_index || !door2_node_index || !door3_node_index || !door4_node_index || !door5_node_index)
		return util::Error("Failed to find door handle nodes by name");
	if (!ceiling_node_index) return util::Error("Failed to find ceiling node by name");

	logic.door1_node_index = *door1_node_index;
	logic.door2_node_index = *door2_node_index;
	logic.door3_node_index = *door3_node_index;
	logic.door4_node_index = *door4_node_index;
	logic.door5_node_index = *door5_node_index;
	logic.ceiling_node_index = *ceiling_node_index;

	auto load_lights_result = logic::load_light_groups(device, model);
	if (!load_lights_result) return load_lights_result.error().forward("Load light groups failed");
	logic.light_groups = std::move(*load_lights_result);

	return logic;
}

std::tuple<render::Params, std::vector<gltf::Drawdata>, std::vector<render::drawdata::Light>> Logic::logic(
	const backend::SDL_context& context,
	const gltf::Model& model
) noexcept
{
	camera_control.update_motion(context);
	const auto camera_matrices = camera_control.update_and_get_matrices();

	if (ImGui::Begin("设置", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		light_control_ui();
		antialias_control_ui();
		statistic_display_ui();
		animation_control_ui();
		light_source_control_ui();
	}
	ImGui::End();

	const auto light_direction = glm::vec3(
		glm::cos(light_pitch) * glm::sin(light_azimuth),
		glm::sin(light_pitch),
		glm::cos(light_pitch) * glm::cos(light_azimuth)
	);

	std::vector<gltf::Animation_key> animation_keys;
	animation_keys.emplace_back("Door1", door1_position * max_door_time);
	animation_keys.emplace_back("Door2", door2_position * max_door_time);
	animation_keys.emplace_back("Door3", door3_position * max_door_time);
	animation_keys.emplace_back("Door4", door4_position * max_door_time);
	animation_keys.emplace_back("Door5", door5_position * max_door5_time);
	animation_keys.emplace_back("CurtainLeft", curtain_left_position * max_curtain_time);
	animation_keys.emplace_back("CurtainRight", curtain_right_position * max_curtain_time);

	std::vector<uint32_t> hidden_nodes;
	if (!show_ceiling) hidden_nodes.push_back(ceiling_node_index);

	std::vector<std::pair<uint32_t, float>> emission_overrides;
	for (const auto& light_group : light_groups | std::views::values)
	{
		emission_overrides.append_range(
			light_group.emission_nodes
			| std::views::transform([mult = light_group.enabled ? 1.0f : 0.0f](uint32_t node_index) {
				  return std::make_pair(node_index, mult);
			  })
		);
	}

	auto main_drawdata =
		model.generate_drawdata(glm::mat4(1.0f), animation_keys, emission_overrides, hidden_nodes);

	for (const auto [idx, door_node_index] :
		 std::to_array(
			 {door1_node_index, door2_node_index, door3_node_index, door4_node_index, door5_node_index}
		 ) | std::views::enumerate)
	{
		const auto handle_node_matrix = main_drawdata.node_matrices[door_node_index];
		draw_handle_hover(camera_matrices, handle_node_matrix, idx);
	}

	auto light_drawdata_list =
		light_groups
		| std::views::values
		| std::views::filter(&logic::Light_group::enabled)
		| std::views::transform(&logic::Light_group::lights)
		| std::views::join
		| std::views::transform([&main_drawdata](const logic::Light_source& light) {
			  return render::drawdata::Light::from(
				  main_drawdata.node_matrices[light.node_index],
				  glm::mat4(1.0),
				  light.light,
				  light.volume
			  );
		  })
		| std::ranges::to<std::vector>();

	std::vector<gltf::Drawdata> drawdata_list;
	drawdata_list.emplace_back(std::move(main_drawdata));

	const render::Primary_light_params primary_light{
		.direction = light_direction,
		.intensity = light_color * light_intensity,
	};

	const render::Bloom_params bloom_params{
		.bloom_attenuation = bloom_attenuation,
		.bloom_strength = bloom_strength,
	};

	const render::Shadow_params shadow_params{
		.csm_linear_blend = csm_linear_blend,
	};

	const render::Params params{
		.aa_mode = aa_mode,
		.camera = camera_matrices,
		.primary_light = primary_light,
		.bloom = bloom_params,
		.shadow = shadow_params,
		.function_mask = {.use_bloom_mask = use_bloom_mask}
	};

	return std::make_tuple(params, std::move(drawdata_list), std::move(light_drawdata_list));
}
