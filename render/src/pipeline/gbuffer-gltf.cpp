#include "render/pipeline/gbuffer-gltf.hpp"
#include "asset/shader/gbuffer-mask.frag.hpp"
#include "asset/shader/gbuffer-skin.vert.hpp"
#include "asset/shader/gbuffer.frag.hpp"
#include "asset/shader/gbuffer.vert.hpp"
#include "gltf/material.hpp"
#include "gltf/model.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "render/target/gbuffer.hpp"
#include "render/target/light.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <ranges>

namespace render::pipeline
{
	namespace
	{
		const auto vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Vertex, position)},
			{.location = 1,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Vertex, normal)  },
			{.location = 2,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Vertex, tangent) },
			{.location = 3,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			 .offset = offsetof(gltf::Vertex, texcoord)},
		});

		const auto vertex_rigged_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Rigged_vertex, position)     },
			{.location = 1,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Rigged_vertex, normal)       },
			{.location = 2,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Rigged_vertex, tangent)      },
			{.location = 3,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			 .offset = offsetof(gltf::Rigged_vertex, texcoord)     },
			{.location = 4,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4,
			 .offset = offsetof(gltf::Rigged_vertex, joint_indices)},
			{.location = 5,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			 .offset = offsetof(gltf::Rigged_vertex, joint_weights)},
		});

		const auto vertex_buffer_descs = std::to_array<SDL_GPUVertexBufferDescription>({
			{.slot = 0,
			 .pitch = sizeof(gltf::Vertex),
			 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			 .instance_step_rate = 0},
		});

		const auto vertex_buffer_rigged_descs = std::to_array<SDL_GPUVertexBufferDescription>({
			{.slot = 0,
			 .pitch = sizeof(gltf::Rigged_vertex),
			 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			 .instance_step_rate = 0},
		});

		const SDL_GPUColorTargetBlendState albedo_color_blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = SDL_GPU_COLORCOMPONENT_R
				| SDL_GPU_COLORCOMPONENT_G
				| SDL_GPU_COLORCOMPONENT_B
				| SDL_GPU_COLORCOMPONENT_A,
			.enable_blend = false,
			.enable_color_write_mask = true,
			.padding1 = 0,
			.padding2 = 0
		};

		const SDL_GPUColorTargetDescription albedo_color_target =
			{.format = target::Gbuffer::albedo_format.format, .blend_state = albedo_color_blend_state};

		const SDL_GPUColorTargetBlendState lighting_info_blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G,
			.enable_blend = false,
			.enable_color_write_mask = true,
			.padding1 = 0,
			.padding2 = 0
		};

		const SDL_GPUColorTargetDescription lighting_info_target = {
			.format = target::Gbuffer::lighting_info_format.format,
			.blend_state = lighting_info_blend_state
		};

		const SDL_GPUColorTargetBlendState light_buffer_blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = SDL_GPU_COLORCOMPONENT_R
				| SDL_GPU_COLORCOMPONENT_G
				| SDL_GPU_COLORCOMPONENT_B
				| SDL_GPU_COLORCOMPONENT_A,
			.enable_blend = false,
			.enable_color_write_mask = true,
			.padding1 = 0,
			.padding2 = 0
		};

		const SDL_GPUColorTargetDescription light_buffer_target = {
			.format = target::Light_buffer::light_buffer_format.format,
			.blend_state = light_buffer_blend_state
		};

		const auto color_target_descs = std::to_array<SDL_GPUColorTargetDescription>(
			{albedo_color_target, lighting_info_target, light_buffer_target}
		);

		gpu::Graphics_pipeline::Depth_stencil_state get_depth_stencil_state(bool double_sided) noexcept
		{
			const auto write_stencil_state = SDL_GPUStencilOpState{
				.fail_op = SDL_GPU_STENCILOP_KEEP,
				.pass_op = SDL_GPU_STENCILOP_REPLACE,
				.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
				.compare_op = SDL_GPU_COMPAREOP_ALWAYS
			};

			const auto no_write_stencil_state = SDL_GPUStencilOpState{
				.fail_op = SDL_GPU_STENCILOP_KEEP,
				.pass_op = SDL_GPU_STENCILOP_KEEP,
				.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
				.compare_op = SDL_GPU_COMPAREOP_ALWAYS
			};

			const auto depth_stencil_state = gpu::Graphics_pipeline::Depth_stencil_state{
				.format = target::Gbuffer::depth_format.format,
				.compare_op = SDL_GPU_COMPAREOP_GREATER,
				.back_stencil_state = double_sided ? write_stencil_state : no_write_stencil_state,
				.front_stencil_state = write_stencil_state,
				.compare_mask = 0xFF,
				.write_mask = 0xFF,
				.enable_depth_test = true,
				.enable_depth_write = true,
				.enable_stencil_test = true
			};

			return depth_stencil_state;
		}

	}

	Gbuffer_gltf::Frag_param Gbuffer_gltf::Frag_param::from(
		const gltf::Material_params::Factor& factor
	) noexcept
	{
		return Frag_param{
			.base_color_factor = factor.base_color_mult,
			.emissive_factor = factor.emissive_mult,
			.metallic_factor = factor.metallic_mult,
			.roughness_factor = factor.roughness_mult,
			.normal_scale = factor.normal_scale,
			.alpha_cutoff = factor.alpha_cutoff,
			.occlusion_strength = factor.occlusion_strength
		};
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_vertex_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		return gpu::Graphics_shader::create(
			device,
			shader_asset::gbuffer_vert,
			gpu::Graphics_shader::Stage::Vertex,
			0,
			0,
			0,
			2
		);
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_vertex_rigged_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		return gpu::Graphics_shader::create(
			device,
			shader_asset::gbuffer_skin_vert,
			gpu::Graphics_shader::Stage::Vertex,
			0,
			0,
			1,
			2
		);
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_fragment_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		return gpu::Graphics_shader::create(
			device,
			shader_asset::gbuffer_frag,
			gpu::Graphics_shader::Stage::Fragment,
			5,
			0,
			0,
			2
		);
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_fragment_mask_shader(
		SDL_GPUDevice* device
	) noexcept
	{
		return gpu::Graphics_shader::create(
			device,
			shader_asset::gbuffer_mask_frag,
			gpu::Graphics_shader::Stage::Fragment,
			5,
			0,
			0,
			2
		);
	}

	static std::expected<gpu::Graphics_pipeline, util::Error> create_pipeline(
		SDL_GPUDevice* device,
		const gpu::Graphics_shader& vertex,
		const gpu::Graphics_shader& vertex_rigged,
		const gpu::Graphics_shader& fragment,
		const gpu::Graphics_shader& fragment_mask,
		gltf::Pipeline_mode mode,
		bool rigged
	) noexcept
	{
		SDL_GPURasterizerState rasterizer_state;
		rasterizer_state.cull_mode = mode.double_sided ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
		rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
		rasterizer_state.depth_bias_clamp = 0;
		rasterizer_state.depth_bias_constant_factor = 0;
		rasterizer_state.depth_bias_slope_factor = 0;
		rasterizer_state.enable_depth_bias = false;
		rasterizer_state.enable_depth_clip = true;

		const gpu::Graphics_shader& fragment_shader =
			(mode.alpha_mode == gltf::Alpha_mode::Opaque) ? fragment : fragment_mask;

		const gpu::Graphics_shader& vertex_shader = rigged ? vertex_rigged : vertex;

		return gpu::Graphics_pipeline::create(
			device,
			vertex_shader,
			fragment_shader,
			SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			SDL_GPU_SAMPLECOUNT_1,
			rasterizer_state,
			rigged ? std::span<const SDL_GPUVertexAttribute>(vertex_rigged_attributes)
				   : std::span<const SDL_GPUVertexAttribute>(vertex_attributes),
			rigged ? vertex_buffer_rigged_descs : vertex_buffer_descs,
			color_target_descs,
			get_depth_stencil_state(mode.double_sided),
			std::format("Gbuffer Gltf Pipeline (mode: {}, rigged: {})", mode.to_string(), rigged)
		);
	}

	std::expected<Gbuffer_gltf, util::Error> Gbuffer_gltf::create(SDL_GPUDevice* device) noexcept
	{
		auto vertex_shader = create_vertex_shader(device);
		if (!vertex_shader) return vertex_shader.error().forward("Create vertex shader failed");

		auto vertex_rigged_shader = create_vertex_rigged_shader(device);
		if (!vertex_rigged_shader)
			return vertex_rigged_shader.error().forward("Create vertex rigged shader failed");

		auto fragment_shader = create_fragment_shader(device);
		if (!fragment_shader) return fragment_shader.error().forward("Create fragment shader failed");

		auto fragment_mask_shader = create_fragment_mask_shader(device);
		if (!fragment_mask_shader)
			return fragment_mask_shader.error().forward("Create fragment mask shader failed");

		std::map<std::pair<gltf::Pipeline_mode, bool>, std::unique_ptr<Gltf_pipeline>> pipeline_result;

		for (const auto [alpha_mode, double_sided, rigged] : std::views::cartesian_product(
				 std::array{gltf::Alpha_mode::Opaque, gltf::Alpha_mode::Mask, gltf::Alpha_mode::Blend},
				 std::array{false, true},
				 std::array{false, true}
			 ))
		{
			const auto pipeline_cfg =
				gltf::Pipeline_mode{.alpha_mode = alpha_mode, .double_sided = double_sided};

			auto pipeline = create_pipeline(
				device,
				*vertex_shader,
				*vertex_rigged_shader,
				*fragment_shader,
				*fragment_mask_shader,
				pipeline_cfg,
				rigged
			);

			if (!pipeline)
				return pipeline.error().forward(
					std::format(
						"Create graphics pipeline failed (alpha_mode: {}, double_sided: {}, rigged: {})",
						static_cast<int>(alpha_mode),
						double_sided,
						rigged
					)
				);

			if (rigged)
				pipeline_result.emplace(
					std::pair(pipeline_cfg, rigged),
					std::make_unique<Pipeline_rigged>(pipeline_cfg, std::move(*pipeline))
				);
			else
				pipeline_result.emplace(
					std::pair(pipeline_cfg, rigged),
					std::make_unique<Pipeline_normal>(pipeline_cfg, std::move(*pipeline))
				);
		}

		return Gbuffer_gltf(std::move(pipeline_result));
	}

	void Gbuffer_gltf::Pipeline_normal::bind(
		const gpu::Command_buffer& command_buffer [[maybe_unused]],
		const gpu::Render_pass& render_pass,
		const glm::mat4& camera_matrix
	) const noexcept
	{
		render_pass.bind_pipeline(pipeline);
		command_buffer.push_uniform_to_vertex(0, util::as_bytes(camera_matrix));
		render_pass.set_stencil_reference(0x01);
	}

	void Gbuffer_gltf::Pipeline_rigged::bind(
		const gpu::Command_buffer& command_buffer [[maybe_unused]],
		const gpu::Render_pass& render_pass,
		const glm::mat4& camera_matrix
	) const noexcept
	{
		render_pass.bind_pipeline(pipeline);
		command_buffer.push_uniform_to_vertex(0, util::as_bytes(camera_matrix));
		render_pass.set_stencil_reference(0x01);
	}

	void Gbuffer_gltf::Pipeline_normal::set_material(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Material_gpu& material
	) const noexcept
	{
		const auto frag_param = Frag_param::from(material.params.factor);

		command_buffer.push_uniform_to_fragment(0, util::as_bytes(frag_param));
		render_pass.bind_fragment_samplers(
			0,
			material.base_color,
			material.normal,
			material.metallic_roughness,
			material.occlusion,
			material.emissive
		);
	}

	void Gbuffer_gltf::Pipeline_rigged::set_material(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Material_gpu& material
	) const noexcept
	{
		const auto frag_param = Frag_param::from(material.params.factor);

		command_buffer.push_uniform_to_fragment(0, util::as_bytes(frag_param));
		render_pass.bind_fragment_samplers(
			0,
			material.base_color,
			material.normal,
			material.metallic_roughness,
			material.occlusion,
			material.emissive
		);
	}

	void Gbuffer_gltf::Pipeline_normal::set_skin(
		const gpu::Render_pass& render_pass [[maybe_unused]],
		const gltf::Deferred_skinning_resource& skinning_resource [[maybe_unused]]
	) const noexcept
	{
		// Do nothing for normal pipelines
	}

	void Gbuffer_gltf::Pipeline_rigged::set_skin(
		const gpu::Render_pass& render_pass [[maybe_unused]],
		const gltf::Deferred_skinning_resource& skinning_resource [[maybe_unused]]
	) const noexcept
	{
		render_pass.bind_vertex_storage_buffers(0, *skinning_resource.joint_matrices_buffer);
	}

	void Gbuffer_gltf::Pipeline_normal::draw(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Primitive_drawcall& drawcall
	) const noexcept
	{
		const auto per_object_param = Per_object_param::from(drawcall);
		command_buffer.push_uniform_to_fragment(1, util::as_bytes(per_object_param));
		const auto transform = drawcall.get_world_transform();
		command_buffer.push_uniform_to_vertex(1, util::as_bytes(transform));

		render_pass.bind_vertex_buffers(0, drawcall.primitive.vertex_buffer_binding);
		render_pass
			.bind_index_buffer(drawcall.primitive.index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		render_pass.draw_indexed(drawcall.primitive.index_count, 0, 1, 0, 0);
	}

	void Gbuffer_gltf::Pipeline_rigged::draw(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Primitive_drawcall& drawcall
	) const noexcept
	{
		const auto per_object_param = Per_object_param::from(drawcall);
		command_buffer.push_uniform_to_fragment(1, util::as_bytes(per_object_param));
		command_buffer.push_uniform_to_vertex(1, util::as_bytes(drawcall.get_joint_matrix_offset()));

		render_pass.bind_vertex_buffers(0, drawcall.primitive.vertex_buffer_binding);
		render_pass
			.bind_index_buffer(drawcall.primitive.index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		render_pass.draw_indexed(drawcall.primitive.index_count, 0, 1, 0, 0);
	}

	void Gbuffer_gltf::render(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& gbuffer_pass,
		const drawdata::Gbuffer& drawdata
	) const noexcept
	{
		command_buffer.push_debug_group("Gbuffer Pass");
		for (const auto& [pipeline_cfg, drawcalls] : drawdata.drawcalls)
		{
			const auto& draw_pipeline = pipelines.at(pipeline_cfg);

			draw_pipeline->bind(command_buffer, gbuffer_pass, drawdata.camera_matrix);

			for (const auto& [drawcall, set_idx, _] : drawcalls)
			{
				const auto& resource_set = drawdata.resource_sets[set_idx];

				draw_pipeline->set_material(
					command_buffer,
					gbuffer_pass,
					resource_set.material_cache[drawcall.material_index]
				);

				if (resource_set.deferred_skinning_resource != nullptr)
					draw_pipeline->set_skin(gbuffer_pass, *resource_set.deferred_skinning_resource);

				draw_pipeline->draw(command_buffer, gbuffer_pass, drawcall);
			}
		}
		command_buffer.pop_debug_group();
	}

	Gbuffer_gltf::Per_object_param Gbuffer_gltf::Per_object_param::from(
		const gltf::Primitive_drawcall& drawcall
	) noexcept
	{
		return Per_object_param{.emissive_multiplier = drawcall.emissive_multiplier};
	}
}