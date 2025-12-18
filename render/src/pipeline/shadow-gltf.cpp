#include "render/pipeline/shadow-gltf.hpp"
#include "render/pass.hpp"

#include "asset/shader/shadow-mask-rigged.vert.hpp"
#include "asset/shader/shadow-mask.frag.hpp"
#include "asset/shader/shadow-mask.vert.hpp"
#include "asset/shader/shadow-rigged.vert.hpp"
#include "asset/shader/shadow.frag.hpp"
#include "asset/shader/shadow.vert.hpp"

#include "gltf/mesh.hpp"
#include "gltf/model.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "render/pipeline/gltf-pipeline.hpp"
#include "render/target/shadow.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <ranges>
#include <span>

namespace render::pipeline
{
	namespace
	{
		const auto vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Shadow_vertex, position)},
		});

		const auto masked_vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Shadow_vertex, position)},
			{.location = 1,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			 .offset = offsetof(gltf::Shadow_vertex, texcoord)},
		});

		const auto rigged_vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, position)     },
			{.location = 1,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, joint_indices)},
			{.location = 2,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, joint_weights)},
		});

		const auto masked_rigged_vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, position)     },
			{.location = 1,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, texcoord)     },
			{.location = 2,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, joint_indices)},
			{.location = 3,
			 .buffer_slot = 0,
			 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			 .offset = offsetof(gltf::Rigged_shadow_vertex, joint_weights)},
		});

		const auto vertex_buffer_descs = std::to_array<SDL_GPUVertexBufferDescription>({
			{.slot = 0,
			 .pitch = sizeof(gltf::Shadow_vertex),
			 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			 .instance_step_rate = 0},
		});

		const auto vertex_buffer_rigged_descs = std::to_array<SDL_GPUVertexBufferDescription>({
			{.slot = 0,
			 .pitch = sizeof(gltf::Rigged_shadow_vertex),
			 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			 .instance_step_rate = 0},
		});

		const auto depth_stencil_state = gpu::Graphics_pipeline::Depth_stencil_state{
			.format = target::Shadow::depth_format.format,
			.compare_op = SDL_GPU_COMPAREOP_GREATER,
			.back_stencil_state = {},
			.front_stencil_state = {},
			.compare_mask = 0xFF,
			.write_mask = 0xFF,
			.enable_depth_test = true,
			.enable_depth_write = true,
			.enable_stencil_test = false
		};

		struct Shaders
		{
			gpu::Graphics_shader vertex;
			gpu::Graphics_shader vertex_mask;
			gpu::Graphics_shader vertex_rigged;
			gpu::Graphics_shader vertex_rigged_mask;
			gpu::Graphics_shader fragment;
			gpu::Graphics_shader fragment_mask;

			static std::expected<Shaders, util::Error> create(SDL_GPUDevice* device) noexcept;
		};

		std::expected<Shaders, util::Error> Shaders::create(SDL_GPUDevice* device) noexcept
		{
			auto vertex_shader = gpu::Graphics_shader::create(
				device,
				shader_asset::shadow_vert,
				gpu::Graphics_shader::Stage::Vertex,
				0,
				0,
				0,
				2
			);

			auto vertex_mask_shader = gpu::Graphics_shader::create(
				device,
				shader_asset::shadow_mask_vert,
				gpu::Graphics_shader::Stage::Vertex,
				0,
				0,
				0,
				2
			);

			auto vertex_rigged_shader = gpu::Graphics_shader::create(
				device,
				shader_asset::shadow_rigged_vert,
				gpu::Graphics_shader::Stage::Vertex,
				0,
				0,
				1,
				2
			);

			auto vertex_rigged_mask_shader = gpu::Graphics_shader::create(
				device,
				shader_asset::shadow_mask_rigged_vert,
				gpu::Graphics_shader::Stage::Vertex,
				0,
				0,
				1,
				2
			);

			auto fragment_shader = gpu::Graphics_shader::create(
				device,
				shader_asset::shadow_frag,
				gpu::Graphics_shader::Stage::Fragment,
				0,
				0,
				0,
				0
			);

			auto fragment_mask_shader = gpu::Graphics_shader::create(
				device,
				shader_asset::shadow_mask_frag,
				gpu::Graphics_shader::Stage::Fragment,
				1,
				0,
				0,
				1
			);

			if (!vertex_shader) return vertex_shader.error().forward("Create Shadow vertex shader failed");
			if (!vertex_mask_shader)
				return vertex_mask_shader.error().forward("Create Shadow Mask vertex shader failed");
			if (!fragment_shader)
				return fragment_shader.error().forward("Create Shadow fragment shader failed");
			if (!fragment_mask_shader)
				return fragment_mask_shader.error().forward("Create Shadow Mask fragment shader failed");

			return Shaders{
				.vertex = std::move(*vertex_shader),
				.vertex_mask = std::move(*vertex_mask_shader),
				.vertex_rigged = std::move(*vertex_rigged_shader),
				.vertex_rigged_mask = std::move(*vertex_rigged_mask_shader),
				.fragment = std::move(*fragment_shader),
				.fragment_mask = std::move(*fragment_mask_shader)
			};
		}

		std::expected<gpu::Graphics_pipeline, util::Error> create_pipeline(
			SDL_GPUDevice* device,
			const Shaders& shaders,
			gltf::Pipeline_mode mode,
			bool rigged
		) noexcept
		{
			SDL_GPURasterizerState rasterizer_state;
			rasterizer_state.cull_mode = mode.double_sided ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
			rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
			rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
			rasterizer_state.depth_bias_clamp = 0.00;
			rasterizer_state.depth_bias_constant_factor = -1e-4;
			rasterizer_state.depth_bias_slope_factor = -2.0;
			rasterizer_state.enable_depth_bias = true;
			rasterizer_state.enable_depth_clip = true;

			const bool masked = (mode.alpha_mode != gltf::Alpha_mode::Opaque);

			// (rigged, masked) -> (vertex attributes, vertex shader)
			const std::map<
				std::tuple<bool, bool>,
				std::tuple<
					std::span<const SDL_GPUVertexAttribute>,
					std::reference_wrapper<const gpu::Graphics_shader>
				>
			>
				vertex_attribute_map = {
					{{false, false}, {vertex_attributes, shaders.vertex}                          },
					{{false, true},  {masked_vertex_attributes, shaders.vertex_mask}              },
					{{true, false},  {rigged_vertex_attributes, shaders.vertex_rigged}            },
					{{true, true},   {masked_rigged_vertex_attributes, shaders.vertex_rigged_mask}},
            };

			const auto& fragment_shader = masked ? shaders.fragment_mask : shaders.fragment;
			const auto& [used_vertex_attributes, vertex_shader] = vertex_attribute_map.at({rigged, masked});
			const auto& used_vertex_buffer_descs = rigged ? vertex_buffer_rigged_descs : vertex_buffer_descs;

			return gpu::Graphics_pipeline::create(
				device,
				vertex_shader,
				fragment_shader,
				SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				SDL_GPU_SAMPLECOUNT_1,
				rasterizer_state,
				used_vertex_attributes,
				used_vertex_buffer_descs,
				{},
				depth_stencil_state,
				std::format("Shadow Gltf Pipeline (mode: {}, rigged: {})", mode.to_string(), rigged)
			);
		}
	}

	std::expected<Shadow_gltf, util::Error> Shadow_gltf::create(SDL_GPUDevice* device) noexcept
	{
		auto shaders = Shaders::create(device);
		if (!shaders) return shaders.error().forward("Create Shadow shaders failed");

		std::map<std::pair<gltf::Pipeline_mode, bool>, std::unique_ptr<Gltf_pipeline>> pipeline_result;

		for (const auto [alpha_mode, double_sided, rigged] : std::views::cartesian_product(
				 std::array{gltf::Alpha_mode::Opaque, gltf::Alpha_mode::Mask, gltf::Alpha_mode::Blend},
				 std::array{false, true},
				 std::array{false, true}
			 ))
		{
			const auto pipeline_cfg =
				gltf::Pipeline_mode{.alpha_mode = alpha_mode, .double_sided = double_sided};

			auto pipeline = create_pipeline(device, *shaders, pipeline_cfg, rigged);

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

		return Shadow_gltf(std::move(pipeline_result));
	}

	void Shadow_gltf::Pipeline_normal::bind(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const glm::mat4& camera_matrix
	) const noexcept
	{
		render_pass.bind_pipeline(pipeline);
		command_buffer.push_uniform_to_vertex(0, util::as_bytes(camera_matrix));
	}

	void Shadow_gltf::Pipeline_rigged::bind(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const glm::mat4& camera_matrix
	) const noexcept
	{
		render_pass.bind_pipeline(pipeline);
		command_buffer.push_uniform_to_vertex(0, util::as_bytes(camera_matrix));
	}

	void Shadow_gltf::Pipeline_normal::set_material(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Material_gpu& material
	) const noexcept
	{
		if (material.params.pipeline.alpha_mode != gltf::Alpha_mode::Opaque)
		{
			const Frag_param frag_param{.alpha_cutoff = material.params.factor.alpha_cutoff};
			const std::array textures = {material.base_color};

			command_buffer.push_uniform_to_fragment(0, util::as_bytes(frag_param));
			render_pass.bind_fragment_samplers(0, textures);
		}
	}

	void Shadow_gltf::Pipeline_rigged::set_material(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Material_gpu& material
	) const noexcept
	{
		if (material.params.pipeline.alpha_mode != gltf::Alpha_mode::Opaque)
		{
			const Frag_param frag_param{.alpha_cutoff = material.params.factor.alpha_cutoff};
			const std::array textures = {material.base_color};

			command_buffer.push_uniform_to_fragment(0, util::as_bytes(frag_param));
			render_pass.bind_fragment_samplers(0, textures);
		}
	}

	void Shadow_gltf::Pipeline_normal::set_skin(
		const gpu::Render_pass& render_pass [[maybe_unused]],
		const gltf::Deferred_skinning_resource& skinning_resource [[maybe_unused]]
	) const noexcept
	{
		// Do nothing
	}

	void Shadow_gltf::Pipeline_rigged::set_skin(
		const gpu::Render_pass& render_pass,
		const gltf::Deferred_skinning_resource& skinning_resource
	) const noexcept
	{
		render_pass.bind_vertex_storage_buffers(0, *skinning_resource.joint_matrices_buffer);
	}

	void Shadow_gltf::Pipeline_normal::draw(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Primitive_drawcall& drawcall
	) const noexcept
	{
		const auto world_transform = drawcall.get_world_transform();

		command_buffer.push_uniform_to_vertex(1, util::as_bytes(world_transform));
		render_pass.bind_vertex_buffers(0, drawcall.primitive.shadow_vertex_buffer_binding);
		render_pass.bind_index_buffer(
			drawcall.primitive.shadow_index_buffer_binding,
			SDL_GPU_INDEXELEMENTSIZE_32BIT
		);
		render_pass.draw_indexed(drawcall.primitive.index_count, 0, 1, 0, 0);
	}

	void Shadow_gltf::Pipeline_rigged::draw(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const gltf::Primitive_drawcall& drawcall
	) const noexcept
	{
		command_buffer.push_uniform_to_vertex(1, util::as_bytes(drawcall.get_joint_matrix_offset()));

		render_pass.bind_vertex_buffers(0, drawcall.primitive.shadow_vertex_buffer_binding);
		render_pass.bind_index_buffer(
			drawcall.primitive.shadow_index_buffer_binding,
			SDL_GPU_INDEXELEMENTSIZE_32BIT
		);
		render_pass.draw_indexed(drawcall.primitive.index_count, 0, 1, 0, 0);
	}

	std::expected<void, util::Error> Shadow_gltf::render(
		const gpu::Command_buffer& command_buffer,
		const target::Shadow& shadow_target,
		const drawdata::Shadow& drawdata
	) const noexcept
	{
		command_buffer.push_debug_group("Shadow Pass");
		for (const auto [level, level_data] : drawdata.csm_levels | std::views::enumerate)
		{
			auto shadow_pass_result = acquire_shadow_pass(command_buffer, shadow_target, level);
			if (!shadow_pass_result)
				return shadow_pass_result.error().forward("Acquire shadow render pass failed");
			auto shadow_pass = std::move(*shadow_pass_result);

			for (const auto& [pipeline_cfg, drawcalls] : level_data.drawcalls)
			{
				const auto& draw_pipeline = pipelines.at(pipeline_cfg);

				draw_pipeline->bind(command_buffer, shadow_pass, level_data.get_vp_matrix());

				for (const auto& [drawcall, set_idx, _] : drawcalls)
				{
					const auto& resource_set = level_data.resource_sets[set_idx];

					draw_pipeline->set_material(
						command_buffer,
						shadow_pass,
						resource_set.material_cache[drawcall.material_index]
					);

					if (resource_set.deferred_skinning_resource != nullptr)
						draw_pipeline->set_skin(shadow_pass, *resource_set.deferred_skinning_resource);

					draw_pipeline->draw(command_buffer, shadow_pass, drawcall);
				}
			}

			shadow_pass.end();
		}
		command_buffer.pop_debug_group();

		return {};
	}
}