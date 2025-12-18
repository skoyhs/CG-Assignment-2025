#include "logic/ambient-light.hpp"

#include <imgui.h>

namespace logic
{
	void Ambient_light::control_ui() noexcept
	{
		ImGui::SliderFloat("环境光", &ambient_intensity, 0.01f, 1.0f, "%.5f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("AO 强度", &ao_strength, 0.0f, 5.0f);
		ImGui::SliderFloat("AO 半径", &ao_radius, 20.0f, 100.0f);
		ImGui::SliderFloat("AO 时域混合", &ao_blend_ratio, 0.01f, 0.1f, "%.3f", ImGuiSliderFlags_Logarithmic);
	}

	render::Ambient_params Ambient_light::get_params() const noexcept
	{
		return render::Ambient_params{
			.intensity = glm::vec3(ambient_intensity),
			.ao_radius = ao_radius,
			.ao_blend_ratio = ao_blend_ratio,
			.ao_strength = ao_strength,
		};
	}
}