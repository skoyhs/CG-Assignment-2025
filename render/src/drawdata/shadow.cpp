#include "render/drawdata/shadow.hpp"
#include "graphics/corner.hpp"
#include "graphics/culling.hpp"
#include "graphics/smallest-bound.hpp"

#include <algorithm>
#include <ranges>

namespace render::drawdata
{
	static glm::vec3 homo_transform(const glm::mat4& mat, const glm::vec3& vec) noexcept
	{
		const glm::vec4 homo = mat * glm::vec4(vec, 1.0f);
		return glm::vec3(homo) / homo.w;
	}

	Shadow::Shadow(
		const glm::mat4& camera_matrix,
		const glm::vec3& light_direction,
		float min_z,
		float linear_blend_ratio
	) noexcept
	{
		const auto camera_mat_inv = glm::inverse(camera_matrix);

		// Compute CSM split points

		const glm::vec3 near_world_pos = homo_transform(camera_mat_inv, {0, 0, 1}),
						far_world_pos = homo_transform(camera_mat_inv, {0, 0, min_z});
		const glm::vec3 linear_split1_world_pos = glm::mix(near_world_pos, far_world_pos, 0.3333),
						linear_split2_world_pos = glm::mix(near_world_pos, far_world_pos, 0.6667);
		const float log_split1_z = glm::mix(1.0f, min_z, 0.3333),
					log_split2_z = glm::mix(1.0f, min_z, 0.6667);
		const glm::vec3 log_split1_world_pos = homo_transform(camera_mat_inv, {0, 0, log_split1_z}),
						log_split2_world_pos = homo_transform(camera_mat_inv, {0, 0, log_split2_z});
		const glm::vec3
			split1_world_pos = glm::mix(log_split1_world_pos, linear_split1_world_pos, linear_blend_ratio),
			split2_world_pos = glm::mix(log_split2_world_pos, linear_split2_world_pos, linear_blend_ratio);
		const float split1_z = homo_transform(camera_matrix, split1_world_pos).z,
					split2_z = homo_transform(camera_matrix, split2_world_pos).z;

		const std::array<float, 4> z_series = {1.0f, split1_z, split2_z, min_z};

		for (auto [level, z_pair] : std::views::zip(csm_levels, z_series | std::views::adjacent<2>))
		{
			const auto [z_near, z_far] = z_pair;

			const auto corners = graphics::transform_corner_points(
				graphics::get_corner_points({-1, -1, z_far}, {1, 1, z_near}),
				camera_mat_inv
			);

			level.smallest_bound = graphics::find_smallest_bound(corners, light_direction);

			const auto temp_vp_matrix =
				glm::ortho(
					level.smallest_bound.left,
					level.smallest_bound.right,
					level.smallest_bound.bottom,
					level.smallest_bound.top,
					0.0f,
					1.0f
				)
				* level.smallest_bound.view_matrix;

			std::ranges::copy(
				graphics::compute_frustum_planes(temp_vp_matrix) | std::views::take(4),
				level.frustum_planes.begin()
			);
		}
	}

	void Shadow::CSM_level_data::append(const gltf::Drawdata& drawdata) noexcept
	{
		const auto current_resource_set_idx = resource_sets.size();
		resource_sets.emplace_back(
			Resource{
				.material_cache = drawdata.material_cache,
				.deferred_skinning_resource = drawdata.deferred_skin_resource
			}
		);

		auto visible_drawcalls =
			drawdata.primitive_drawcalls | std::views::filter([this](const auto& drawcall) -> bool {
				return graphics::box_in_frustum(
					drawcall.world_position_min,
					drawcall.world_position_max,
					frustum_planes
				);
			});

		/* Process Non-rigged Drawcalls */

		for (const auto& drawcall : visible_drawcalls)
		{
			const auto& pipeline_mode = drawdata.material_cache[drawcall.material_index].params.pipeline;
			auto& target = drawcalls[std::pair(pipeline_mode, drawcall.is_rigged())];

			const auto corners_world =
				graphics::get_corner_points(drawcall.world_position_min, drawcall.world_position_max);
			const auto corners_light_view =
				graphics::transform_corner_points(corners_world, smallest_bound.view_matrix);

			const auto [min_z, max_z] = std::ranges::minmax(corners_light_view, {}, &glm::vec3::z);
			near = std::min(near, -max_z.z);
			far = std::max(far, -min_z.z);

			if (target.empty()) target.reserve(1024);

			target.emplace_back(
				Drawcall{
					.drawcall = drawcall,
					.resource_set_index = current_resource_set_idx,
					.min_z = -min_z.z
				}
			);
		}
	}

	glm::mat4 Shadow::CSM_level_data::get_vp_matrix() const noexcept
	{
		const auto projection_matrix = glm::ortho(
			smallest_bound.left,
			smallest_bound.right,
			smallest_bound.bottom,
			smallest_bound.top,
			near,
			far
		);

		return projection_matrix * smallest_bound.view_matrix;
	}

	void Shadow::CSM_level_data::sort() noexcept
	{
		for (auto& drawcall_vec : drawcalls | std::views::values)
			std::ranges::sort(drawcall_vec, {}, &Drawcall::min_z);
	}

	void Shadow::append(const gltf::Drawdata& drawdata) noexcept
	{
		for (auto& level : csm_levels) level.append(drawdata);
	}

	void Shadow::sort() noexcept
	{
		for (auto& level : csm_levels) level.sort();
	}

	glm::mat4 Shadow::get_vp_matrix(size_t level) const noexcept
	{
		return csm_levels[level].get_vp_matrix();
	}
}