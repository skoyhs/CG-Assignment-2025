#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "gpu/render-pass.hpp"
#include "gpu/sampler.hpp"
#include "render/drawdata/light.hpp"
#include "render/target/gbuffer.hpp"
#include "render/target/light.hpp"

#include <expected>

namespace render::pipeline
{
	///
	/// @brief Render point light
	///
	///
	class Light
	{
	  public:

		struct Param
		{
			glm::mat4 camera_view_projection;
			glm::vec3 eye_position;
			glm::u32vec2 screen_size;
		};

		static std::expected<Light, util::Error> create(SDL_GPUDevice* device) noexcept;

		///
		/// @brief Render primary point light, use dedicated render pass for each light
		///
		/// @param command_buffer GPU command buffer
		/// @param param Parameters
		///
		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer_target,
			const target::Light_buffer& light_buffer_target,
			std::span<const drawdata::Light> drawdata,
			const Param& param
		) const noexcept;

	  private:

		struct Point_light_param
		{
			glm::mat4 VP_inv;
			glm::vec2 screen_size;
			alignas(16) glm::vec3 eye_position;
			alignas(16) glm::vec3 light_position;
			alignas(16) glm::vec3 rel_intensity;
			float range;

			static Point_light_param from(const drawdata::Light& drawdata, const Param& param) noexcept;
		};

		struct Spot_light_param
		{
			glm::mat4 VP_inv;
			glm::mat4 M_inv;
			glm::vec2 screen_size;
			alignas(16) glm::vec3 eye_position;
			alignas(16) glm::vec3 light_position;
			alignas(16) glm::vec3 rel_intensity;
			float range;
			float inner_cone_cos;
			float outer_cone_cos;

			static Spot_light_param from(const drawdata::Light& drawdata, const Param& param) noexcept;
		};

		std::expected<void, util::Error> run_depth_only_pass(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer_target,
			const std::function<void(const gpu::Render_pass& render_pass)>& task
		) const noexcept;

		std::expected<void, util::Error> run_light_pass(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer_target,
			const target::Light_buffer& light_buffer_target,
			const std::function<void(const gpu::Render_pass& render_pass)>& task
		) const noexcept;

		gpu::Graphics_pipeline depth_test_pipeline;

		// Pipeline for point light when camera is outside light volume
		gpu::Graphics_pipeline point_light_outside_pipeline;

		// Pipeline for point light when camera is inside light volume
		gpu::Graphics_pipeline point_light_inside_pipeline;

		gpu::Graphics_pipeline spot_light_outside_pipeline;

		gpu::Graphics_pipeline spot_light_inside_pipeline;

		gpu::Sampler nearest_sampler;

		Light(
			gpu::Graphics_pipeline depth_test_pipeline,
			gpu::Graphics_pipeline point_light_outside_pipeline,
			gpu::Graphics_pipeline point_light_inside_pipeline,
			gpu::Graphics_pipeline spot_light_outside_pipeline,
			gpu::Graphics_pipeline spot_light_inside_pipeline,
			gpu::Sampler nearest_sampler
		) noexcept :
			depth_test_pipeline(std::move(depth_test_pipeline)),
			point_light_outside_pipeline(std::move(point_light_outside_pipeline)),
			point_light_inside_pipeline(std::move(point_light_inside_pipeline)),
			spot_light_outside_pipeline(std::move(spot_light_outside_pipeline)),
			spot_light_inside_pipeline(std::move(spot_light_inside_pipeline)),
			nearest_sampler(std::move(nearest_sampler))
		{}

	  public:

		Light(const Light&) = delete;
		Light(Light&&) = default;
		Light& operator=(const Light&) = delete;
		Light& operator=(Light&&) = default;
	};
}