#include "render/light-volume.hpp"
#include "graphics/util/quick-create.hpp"
#include "util/as-byte.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/vector_relational.hpp>
#include <ranges>

#include <print>

namespace render
{
	std::expected<Light_volume, util::Error> Light_volume::from_model(
		SDL_GPUDevice* device,
		const wavefront::Object& object,
		const std::string& name
	) noexcept
	{
		const auto position_data =
			object.vertices | std::views::transform(&wavefront::Vertex::pos) | std::ranges::to<std::vector>();

		const auto [min, max] = std::ranges::fold_left(
			position_data,
			std::make_pair(glm::vec3{FLT_MAX}, glm::vec3{-FLT_MAX}),
			[](const auto& acc, const auto& pos) {
				return std::make_pair(glm::min(acc.first, pos), glm::max(acc.second, pos));
			}
		);

		auto vertex_buffer = graphics::create_buffer_from_data(
			device,
			{.vertex = true},
			util::as_bytes(position_data),
			std::format("{} Vertex Buffer", name)
		);
		if (!vertex_buffer) return vertex_buffer.error().forward("Create light volume vertex buffer failed");

		auto tri_positions = position_data | std::views::stride(3) | std::ranges::to<std::vector>();
		auto tri_normals =
			position_data
			| std::views::chunk(3)
			| std::views::transform([](const auto& tri_pos) {
				  const glm::vec3 edge1 = tri_pos[1] - tri_pos[0];
				  const glm::vec3 edge2 = tri_pos[2] - tri_pos[0];
				  return glm::normalize(glm::cross(edge1, edge2));
			  })
			| std::ranges::to<std::vector>();

		return Light_volume{
			.vertex_buffer = std::move(*vertex_buffer),
			.vertex_count = static_cast<uint32_t>(position_data.size()),
			.min = min,
			.max = max,
			.tri_positions = std::move(tri_positions),
			.tri_normals = std::move(tri_normals)
		};
	}

	bool Light_volume::camera_inside(glm::vec3 local_eye_position) const noexcept
	{
		if (glm::any(glm::lessThan(local_eye_position, min))
			|| glm::any(glm::greaterThan(local_eye_position, max)))
			return false;

		return std::ranges::all_of(
			std::views::zip_transform(
				[local_eye_position](auto tri_pos, auto tri_normal) {
					const glm::vec3 to_eye = local_eye_position - tri_pos;
					return glm::dot(tri_normal, to_eye) < 0.0f;
				},
				tri_positions,
				tri_normals
			),
			[](bool v) { return v; }
		);
	}
}