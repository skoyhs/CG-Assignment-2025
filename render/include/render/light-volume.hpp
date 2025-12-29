#pragma once

#include "gpu/buffer.hpp"
#include <glm/glm.hpp>
#include <wavefront.hpp>

namespace render
{
	struct Light_volume
	{
		gpu::Buffer vertex_buffer;
		uint32_t vertex_count;

		glm::vec3 min, max;  // min/max of position, local space

		std::vector<glm::vec3> tri_positions;
		std::vector<glm::vec3> tri_normals;

		static std::expected<Light_volume, util::Error> from_model(
			SDL_GPUDevice* device,
			const wavefront::Object& object,
			const std::string& name
		) noexcept;

		///
		/// @brief Calculate whether camera is inside the volume
		///
		/// @param local_eye_position Eye position in local space
		/// @return `true` if camera is inside the volume
		///
		bool camera_inside(glm::vec3 local_eye_position) const noexcept;
	};
}