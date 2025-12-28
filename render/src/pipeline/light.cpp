#include "render/pipeline/light.hpp"

#include "asset/shader/light-volume-depth.frag.hpp"
#include "asset/shader/light-volume.vert.hpp"
#include "asset/shader/point-light.frag.hpp"
#include "asset/shader/spot-light.frag.hpp"
#include "gltf/light.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "graphics/culling.hpp"
#include "render/const-params.hpp"
#include "render/target/gbuffer.hpp"
#include "util/as-byte.hpp"
#include "util/error.hpp"
#include <SDL3/SDL_gpu.h>

namespace render::pipeline
{
	static constexpr std::array vertex_attribute = std::to_array<SDL_GPUVertexAttribute>({
		{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0},
	});

	static constexpr std::array vertex_buffer_desc = std::to_array<SDL_GPUVertexBufferDescription>({
		{.slot = 0,
		 .pitch = sizeof(glm::vec3),
		 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		 .instance_step_rate = 0}
	});

	static std::expected<gpu::Graphics_shader, util::Error> create_vertex_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		const auto vertex_shader_data = shader_asset::light_volume_vert;
		return gpu::Graphics_shader::
			create(device, vertex_shader_data, gpu::Graphics_shader::Stage::Vertex, 0, 0, 0, 1)
				.transform_error(util::Error::forward_fn("Create point-light vertex shader failed"));
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_depth_test_fragment_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		const auto fragment_shader_data = shader_asset::light_volume_depth_frag;
		return gpu::Graphics_shader::create(
				   device,
				   fragment_shader_data,
				   gpu::Graphics_shader::Stage::Fragment,
				   0,
				   0,
				   0,
				   0
		)
			.transform_error(util::Error::forward_fn("Create point-light depth test fragment shader failed"));
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_point_light_fragment_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		const auto fragment_shader_data = shader_asset::point_light_frag;
		return gpu::Graphics_shader::
			create(device, fragment_shader_data, gpu::Graphics_shader::Stage::Fragment, 3, 0, 0, 1)
				.transform_error(util::Error::forward_fn("Create point-light light fragment shader failed"));
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_spot_light_fragment_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		const auto fragment_shader_data = shader_asset::spot_light_frag;
		return gpu::Graphics_shader::
			create(device, fragment_shader_data, gpu::Graphics_shader::Stage::Fragment, 3, 0, 0, 1)
				.transform_error(util::Error::forward_fn("Create spot-light light fragment shader failed"));
	}

	static std::expected<gpu::Graphics_pipeline, util::Error> create_depth_test_pipeline(
		SDL_GPUDevice* device
	) noexcept
	{
		static constexpr SDL_GPURasterizerState rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_FRONT,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
			.depth_bias_constant_factor = 0,
			.depth_bias_clamp = 0,
			.depth_bias_slope_factor = 0,
			.enable_depth_bias = false,
			.enable_depth_clip = true,
			.padding1 = 0,
			.padding2 = 0
		};

		static constexpr SDL_GPUStencilOpState stencil_state = {
			.fail_op = SDL_GPU_STENCILOP_KEEP,
			.pass_op = SDL_GPU_STENCILOP_REPLACE,
			.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
			.compare_op = SDL_GPU_COMPAREOP_ALWAYS
		};

		static constexpr gpu::Graphics_pipeline::Depth_stencil_state depth_stencil_state = {
			.format = target::Gbuffer::depth_format.format,
			.compare_op = SDL_GPU_COMPAREOP_LESS,
			.back_stencil_state = stencil_state,
			.compare_mask = 0xff,
			.write_mask = 0x01,
			.enable_depth_test = true,
			.enable_depth_write = false,
			.enable_stencil_test = true
		};

		auto vertex_shader = create_vertex_shader(device);
		auto fragment_shader = create_depth_test_fragment_shader(device);

		if (!vertex_shader) return vertex_shader.error();
		if (!fragment_shader) return fragment_shader.error();

		return gpu::Graphics_pipeline::create(
				   device,
				   *vertex_shader,
				   *fragment_shader,
				   SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				   SDL_GPU_SAMPLECOUNT_1,
				   rasterizer_state,
				   vertex_attribute,
				   vertex_buffer_desc,
				   {},
				   depth_stencil_state,
				   "Point Light Primary Depth Test Pipeline"
		)
			.transform_error(util::Error::forward_fn("Create depth test pipeline failed"));
	}

	static std::expected<gpu::Graphics_pipeline, util::Error> create_light_pipeline(
		SDL_GPUDevice* device,
		gltf::Light::Type type,
		bool inside
	) noexcept
	{
		static constexpr SDL_GPURasterizerState outside_rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
			.depth_bias_constant_factor = 0,
			.depth_bias_clamp = 0,
			.depth_bias_slope_factor = 0,
			.enable_depth_bias = false,
			.enable_depth_clip = false,
			.padding1 = 0,
			.padding2 = 0
		};

		static constexpr SDL_GPURasterizerState inside_rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_FRONT,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
			.depth_bias_constant_factor = 0,
			.depth_bias_clamp = 0,
			.depth_bias_slope_factor = 0,
			.enable_depth_bias = false,
			.enable_depth_clip = false,
			.padding1 = 0,
			.padding2 = 0
		};

		static constexpr SDL_GPUStencilOpState outside_stencil_state = {
			.fail_op = SDL_GPU_STENCILOP_KEEP,
			.pass_op = SDL_GPU_STENCILOP_KEEP,
			.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
			.compare_op = SDL_GPU_COMPAREOP_EQUAL
		};

		static constexpr gpu::Graphics_pipeline::Depth_stencil_state outside_depth_stencil_state = {
			.format = target::Gbuffer::depth_format.format,
			.compare_op = SDL_GPU_COMPAREOP_GREATER,
			.front_stencil_state = outside_stencil_state,
			.compare_mask = 0x01,
			.write_mask = 0x00,
			.enable_depth_test = true,
			.enable_depth_write = false,
			.enable_stencil_test = true
		};

		// When camera is inside light volume, draw backfaces only with depth test
		static constexpr gpu::Graphics_pipeline::Depth_stencil_state inside_depth_stencil_state = {
			.format = target::Gbuffer::depth_format.format,
			.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
			.compare_mask = 0x00,
			.write_mask = 0x00,
			.enable_depth_test = true,
			.enable_depth_write = false,
			.enable_stencil_test = false
		};

		static constexpr SDL_GPUColorTargetBlendState blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = 0,
			.enable_blend = true,
			.enable_color_write_mask = false,
			.padding1 = 0,
			.padding2 = 0
		};

		static constexpr std::array color_target_descs = std::to_array<SDL_GPUColorTargetDescription>({
			{.format = target::Light_buffer::light_buffer_format.format, .blend_state = blend_state}
		});

		auto vertex_shader = create_vertex_shader(device);
		auto fragment_shader = [device, type] -> std::expected<gpu::Graphics_shader, util::Error> {
			switch (type)
			{
			case gltf::Light::Type::Point:
				return create_point_light_fragment_shader(device);
			case gltf::Light::Type::Spot:
				return create_spot_light_fragment_shader(device);
			default:
				return util::Error("Unsupported light type");
			}
		}();
		const auto pipeline_name = std::format(
			"Light pipeline ({}, {})",
			type == gltf::Light::Type::Point ? "Point" : "Spot",
			inside ? "Inside" : "Outside"
		);

		if (!vertex_shader) return vertex_shader.error().forward("Create light vertex shader failed");
		if (!fragment_shader) return fragment_shader.error().forward("Create light fragment shader failed");

		return gpu::Graphics_pipeline::create(
				   device,
				   *vertex_shader,
				   *fragment_shader,
				   SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				   SDL_GPU_SAMPLECOUNT_1,
				   inside ? inside_rasterizer_state : outside_rasterizer_state,
				   vertex_attribute,
				   vertex_buffer_desc,
				   color_target_descs,
				   inside ? inside_depth_stencil_state : outside_depth_stencil_state,
				   pipeline_name
		)
			.transform_error(util::Error::forward_fn("Create light pipeline failed"));
	}

	Light::Point_light_param Light::Point_light_param::from(
		const drawdata::Light& drawdata,
		const Param& param
	) noexcept
	{
		return {
			.VP_inv = glm::inverse(param.camera_view_projection),
			.screen_size = glm::vec2(param.screen_size),
			.eye_position = param.eye_position,
			.light_position = glm::vec3(drawdata.node_transform[3]),
			.rel_intensity = drawdata.light.emission / REF_LUMINANCE,
			.range = drawdata.light.range
		};
	}

	Light::Spot_light_param Light::Spot_light_param::from(
		const drawdata::Light& drawdata,
		const Param& param
	) noexcept
	{
		return {
			.VP_inv = glm::inverse(param.camera_view_projection),
			.M_inv = glm::inverse(drawdata.node_transform),
			.screen_size = glm::vec2(param.screen_size),
			.eye_position = param.eye_position,
			.light_position = glm::vec3(drawdata.node_transform[3]),
			.rel_intensity = drawdata.light.emission / REF_LUMINANCE,
			.range = drawdata.light.range,
			.inner_cone_cos = glm::cos(drawdata.light.inner_cone_angle),
			.outer_cone_cos = glm::cos(drawdata.light.outer_cone_angle)
		};
	}

	std::expected<Light, util::Error> Light::create(SDL_GPUDevice* device) noexcept
	{
		auto depth_test_pipeline = create_depth_test_pipeline(device);
		auto point_light_pipeline_outside = create_light_pipeline(device, gltf::Light::Type::Point, false);
		auto point_light_pipeline_inside = create_light_pipeline(device, gltf::Light::Type::Point, true);
		auto spot_light_pipeline_outside = create_light_pipeline(device, gltf::Light::Type::Spot, false);
		auto spot_light_pipeline_inside = create_light_pipeline(device, gltf::Light::Type::Spot, true);

		if (!depth_test_pipeline)
			return depth_test_pipeline.error().forward(
				"Create point light primary depth test pipeline failed"
			);
		if (!point_light_pipeline_outside)
			return point_light_pipeline_outside.error().forward(
				"Create point light primary light pipeline failed"
			);
		if (!point_light_pipeline_inside)
			return point_light_pipeline_inside.error().forward(
				"Create point light primary light (inside) pipeline failed"
			);

		auto nearest_sampler = gpu::Sampler::create(
			device,
			gpu::Sampler::Create_info{
				.min_filter = gpu::Sampler::Filter::Nearest,
				.mag_filter = gpu::Sampler::Filter::Nearest,
				.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
				.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
				.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge,
				.address_mode_w = gpu::Sampler::Address_mode::Clamp_to_edge,
				.min_lod = 0.0f,
				.max_lod = 0.0f,
				.mip_lod_bias = 0.0f,
			}
		);
		if (!nearest_sampler)
			return nearest_sampler.error().forward("Create point light nearest sampler failed");

		return Light(
			std::move(*depth_test_pipeline),
			std::move(*point_light_pipeline_outside),
			std::move(*point_light_pipeline_inside),
			std::move(*spot_light_pipeline_outside),
			std::move(*spot_light_pipeline_inside),
			std::move(*nearest_sampler)
		);
	}

	std::expected<void, util::Error> Light::render(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer_target,
		const target::Light_buffer& light_buffer_target,
		std::span<const drawdata::Light> drawdata,
		const Param& param
	) const noexcept
	{
		const auto camera_frustum = graphics::compute_frustum_planes(param.camera_view_projection);

		const auto visible_drawdata =
			drawdata
			| std::views::filter([&](const drawdata::Light& drawcall) {
				  const auto [min, max] = graphics::local_bound_to_world(
					  drawcall.volume->min,
					  drawcall.volume->max,
					  drawcall.volume_transform
				  );
				  return graphics::box_in_frustum(min, max, camera_frustum);
			  })
			| std::ranges::to<std::vector>();

		command_buffer.push_debug_group("Render light stencil test");
		auto depth_only_pass_result = run_depth_only_pass(
			command_buffer,
			gbuffer_target,
			[&, this](const gpu::Render_pass& render_pass) noexcept {
				render_pass.bind_pipeline(depth_test_pipeline);
				render_pass.set_stencil_reference(0x01);

				for (const auto& drawcall : visible_drawdata)
				{
					const auto local_eye_position = glm::vec3(
						glm::inverse(drawcall.volume_transform) * glm::vec4(param.eye_position, 1.0f)
					);
					if (drawcall.volume->camera_inside(local_eye_position)) continue;

					const auto mvp = param.camera_view_projection * drawcall.volume_transform;
					command_buffer.push_uniform_to_vertex(0, util::as_bytes(mvp));

					render_pass.bind_vertex_buffers(
						0,
						SDL_GPUBufferBinding{.buffer = drawcall.volume->vertex_buffer, .offset = 0}
					);
					render_pass.draw(drawcall.volume->vertex_count, 0, 1, 0);
				}
			}
		);
		command_buffer.pop_debug_group();

		if (!depth_only_pass_result)
			return depth_only_pass_result.error().forward("Acquire depth only pass failed");

		const auto albedo_binding = gbuffer_target.albedo_texture->bind_with_sampler(nearest_sampler);
		const auto lighting_info_binding =
			gbuffer_target.lighting_info_texture->bind_with_sampler(nearest_sampler);
		const auto depth_binding =
			gbuffer_target.depth_value_texture.current().bind_with_sampler(nearest_sampler);

		command_buffer.push_debug_group("Render light pass");
		auto light_pass_result = run_light_pass(
			command_buffer,
			gbuffer_target,
			light_buffer_target,
			[&, this](const gpu::Render_pass& render_pass) noexcept {
				render_pass.set_stencil_reference(0x01);

				for (const auto& drawcall : visible_drawdata)
				{
					const auto local_eye_position = glm::vec3(
						glm::inverse(drawcall.volume_transform) * glm::vec4(param.eye_position, 1.0f)
					);
					const bool inside = drawcall.volume->camera_inside(local_eye_position);

					const auto mvp = param.camera_view_projection * drawcall.volume_transform;
					command_buffer.push_uniform_to_vertex(0, util::as_bytes(mvp));

					if (drawcall.light.type == gltf::Light::Type::Point)
					{
						render_pass.bind_pipeline(
							inside ? point_light_inside_pipeline : point_light_outside_pipeline
						);
						render_pass
							.bind_fragment_samplers(0, albedo_binding, lighting_info_binding, depth_binding);

						const auto point_light_param = Point_light_param::from(drawcall, param);
						command_buffer.push_uniform_to_fragment(0, util::as_bytes(point_light_param));

						render_pass.bind_vertex_buffers(
							0,
							SDL_GPUBufferBinding{.buffer = drawcall.volume->vertex_buffer, .offset = 0}
						);
						render_pass.draw(drawcall.volume->vertex_count, 0, 1, 0);
					}

					if (drawcall.light.type == gltf::Light::Type::Spot)
					{
						render_pass.bind_pipeline(
							inside ? spot_light_inside_pipeline : spot_light_outside_pipeline
						);
						render_pass
							.bind_fragment_samplers(0, albedo_binding, lighting_info_binding, depth_binding);

						const auto spot_light_param = Spot_light_param::from(drawcall, param);
						command_buffer.push_uniform_to_fragment(0, util::as_bytes(spot_light_param));

						render_pass.bind_vertex_buffers(
							0,
							SDL_GPUBufferBinding{.buffer = drawcall.volume->vertex_buffer, .offset = 0}
						);
						render_pass.draw(drawcall.volume->vertex_count, 0, 1, 0);
					}
				}
			}
		);
		command_buffer.pop_debug_group();

		if (!light_pass_result) return light_pass_result.error().forward("Acquire light pass failed");

		return {};
	}

	std::expected<void, util::Error> Light::run_depth_only_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer_target,
		const std::function<void(const gpu::Render_pass& render_pass)>& task
	) const noexcept
	{
		const SDL_GPUDepthStencilTargetInfo depth_target_info = {
			.texture = *gbuffer_target.depth_texture,
			.clear_depth = 1.0f,
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
			.stencil_store_op = SDL_GPU_STOREOP_STORE,
			.cycle = false,
			.clear_stencil = 0,
			.mip_level = 0,
			.layer = 0
		};

		return command_buffer.run_render_pass({}, depth_target_info, task);
	}

	std::expected<void, util::Error> Light::run_light_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer_target,
		const target::Light_buffer& light_buffer_target,
		const std::function<void(const gpu::Render_pass& render_pass)>& task
	) const noexcept
	{
		const SDL_GPUColorTargetInfo light_buffer_target_info = {
			.texture = light_buffer_target.light_texture.current(),
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {.r = 0, .g = 0, .b = 0, .a = 0},
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
			.resolve_texture = nullptr,
			.resolve_mip_level = 0,
			.resolve_layer = 0,
			.cycle = false,
			.cycle_resolve_texture = false,
			.padding1 = 0,
			.padding2 = 0
		};

		const SDL_GPUDepthStencilTargetInfo depth_target_info = {
			.texture = *gbuffer_target.depth_texture,
			.clear_depth = 1.0f,
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_load_op = SDL_GPU_LOADOP_LOAD,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.cycle = false,
			.clear_stencil = 0,
			.mip_level = 0,
			.layer = 0
		};

		return command_buffer.run_render_pass(
			std::span<const SDL_GPUColorTargetInfo>(&light_buffer_target_info, 1),
			depth_target_info,
			task
		);
	}
}