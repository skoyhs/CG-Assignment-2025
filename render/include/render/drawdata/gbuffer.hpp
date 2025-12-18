#pragma once

#include "gltf/material.hpp"
#include "gltf/model.hpp"

namespace render::drawdata
{
	struct Gbuffer
	{
		struct Drawcall
		{
			gltf::Primitive_drawcall drawcall;
			size_t resource_set_index;
			float max_z;
		};

		struct Resource
		{
			gltf::Material_cache::Ref material_cache;
			std::shared_ptr<gltf::Deferred_skinning_resource> deferred_skinning_resource;
		};

		std::map<std::pair<gltf::Pipeline_mode, bool>, std::vector<Drawcall>> drawcalls;
		std::vector<Resource> resource_sets;

		glm::mat4 camera_matrix;
		glm::vec3 eye_position;
		glm::vec3 eye_to_nearplane;

		std::array<glm::vec4, 6> frustum_planes;
		float min_z = 1;      // Minimum Z value
		float near_distance;  // Distance from eye to near plane

		///
		/// @brief Create drawdata with camera matrix
		///
		/// @param camera_matrix Camera matrix
		///
		Gbuffer(const glm::mat4& camera_matrix, const glm::vec3& eye_position) noexcept;

		///
		/// @brief Add glTF drawdata
		///
		/// @param drawdata glTF drawdata
		///
		void append(const gltf::Drawdata& drawdata) noexcept;

		///
		/// @brief Get maximum z depth
		///
		/// @return Maximum z depth
		///
		float get_min_z() const noexcept;

		///
		/// @brief Get absolute maximum distance of all drawcalls
		/// @note Calculated from camera position to the farthest visible point at min-z plane
		/// @return Maximum distance
		///
		float get_max_distance() const noexcept;

		///
		/// @brief Sort drawcalls for optimal rendering
		///
		///
		void sort() noexcept;
	};
}