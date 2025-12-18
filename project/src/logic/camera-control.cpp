#include "logic/camera-control.hpp"
#include "render/param.hpp"

#include <imgui.h>

namespace logic
{
	void Camera::update() noexcept
	{
		const auto [width, height] = ImGui::GetIO().DisplaySize;

		if (!ImGui::GetIO().WantCaptureMouse)
		{
			const auto delta = ImGui::GetIO().MouseDelta;
			if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				rotate_controller.rotate(camera_orbit, {float(width), float(height)}, {delta.x, delta.y});
			}

			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				pan_controller.pan(camera_orbit, {float(width), float(height)}, {delta.x, delta.y});
			}

			const auto mouse_wheel = ImGui::GetIO().MouseWheel;
			if (mouse_wheel != 0.0f) camera_orbit.distance *= glm::pow(0.8f, mouse_wheel);
		}
	}

	render::Camera_matrices Camera::get_matrices() noexcept
	{
		const auto [width, height] = ImGui::GetIO().DisplaySize;
		const auto aspect_ratio = width / height;

		const auto view_matrix = camera_orbit.matrix();
		const auto proj_matrix = camera_projection.matrix_reverse_z(aspect_ratio);
		const auto camera_matrix = proj_matrix * view_matrix;
		const auto prev_matrix = prev_frame_camera_matrix.value_or(camera_matrix);
		prev_frame_camera_matrix = camera_matrix;

		return render::Camera_matrices{
			.view_matrix = view_matrix,
			.proj_matrix = proj_matrix,
			.prev_view_proj_matrix = prev_matrix,
			.eye_position = camera_orbit.eye_position(),
		};
	}
}