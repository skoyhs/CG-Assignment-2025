#pragma once

#include "gltf/material.hpp"
#include "gltf/model.hpp"
#include "gltf/skin.hpp"
#include "gpu/command-buffer.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "gpu/render-pass.hpp"
#include "render/drawdata/gbuffer.hpp"
#include "render/pipeline/gltf-pipeline.hpp"

#include <expected>

namespace render::pipeline
{
	class Gbuffer_gltf
	{
		// (Pipeline Mode, Rigged) -> Pipeline Instance
		std::map<std::pair<gltf::Pipeline_mode, bool>, std::unique_ptr<Gltf_pipeline>> pipelines;

		struct alignas(64) Frag_param
		{
			alignas(16) glm::vec4 base_color_factor;
			alignas(16) glm::vec3 emissive_factor;
			alignas(4) float metallic_factor;
			alignas(4) float roughness_factor;
			alignas(4) float normal_scale;
			alignas(4) float alpha_cutoff;
			alignas(4) float occlusion_strength;

			static Frag_param from(const gltf::Material_params::Factor& factor) noexcept;
		};

		struct Per_object_param
		{
			alignas(4) float emissive_multiplier;

			static Per_object_param from(const gltf::Primitive_drawcall& drawcall) noexcept;
		};

		Gbuffer_gltf(
			std::map<std::pair<gltf::Pipeline_mode, bool>, std::unique_ptr<Gltf_pipeline>> pipelines
		) noexcept :
			pipelines(std::move(pipelines))
		{}

		class Pipeline_normal : public Gltf_pipeline
		{
			gltf::Pipeline_mode mode;
			gpu::Graphics_pipeline pipeline;

		  public:

			Pipeline_normal(gltf::Pipeline_mode mode, gpu::Graphics_pipeline pipeline) :
				mode(mode),
				pipeline(std::move(pipeline))
			{}

			Pipeline_normal(const Pipeline_normal&) = delete;
			Pipeline_normal(Pipeline_normal&&) = default;
			Pipeline_normal& operator=(const Pipeline_normal&) = delete;
			Pipeline_normal& operator=(Pipeline_normal&&) = default;

			void bind(
				const gpu::Command_buffer& command_buffer,
				const gpu::Render_pass& render_pass,
				const glm::mat4& camera_matrix
			) const noexcept override;

			void set_material(
				const gpu::Command_buffer& command_buffer,
				const gpu::Render_pass& render_pass,
				const gltf::Material_gpu& material
			) const noexcept override;

			void set_skin(
				const gpu::Render_pass& render_pass,
				const gltf::Deferred_skinning_resource& skinning_resource
			) const noexcept override;

			void draw(
				const gpu::Command_buffer& command_buffer,
				const gpu::Render_pass& render_pass,
				const gltf::Primitive_drawcall& drawcall
			) const noexcept override;
		};

		class Pipeline_rigged : public Gltf_pipeline
		{
			gltf::Pipeline_mode mode;
			gpu::Graphics_pipeline pipeline;

		  public:

			Pipeline_rigged(gltf::Pipeline_mode mode, gpu::Graphics_pipeline pipeline) :
				mode(mode),
				pipeline(std::move(pipeline))
			{}

			Pipeline_rigged(const Pipeline_rigged&) = delete;
			Pipeline_rigged(Pipeline_rigged&&) = default;
			Pipeline_rigged& operator=(const Pipeline_rigged&) = delete;
			Pipeline_rigged& operator=(Pipeline_rigged&&) = default;

			void bind(
				const gpu::Command_buffer& command_buffer,
				const gpu::Render_pass& render_pass,
				const glm::mat4& camera_matrix
			) const noexcept override;

			void set_material(
				const gpu::Command_buffer& command_buffer,
				const gpu::Render_pass& render_pass,
				const gltf::Material_gpu& material
			) const noexcept override;

			void set_skin(
				const gpu::Render_pass& render_pass,
				const gltf::Deferred_skinning_resource& skinning_resource
			) const noexcept override;

			void draw(
				const gpu::Command_buffer& command_buffer,
				const gpu::Render_pass& render_pass,
				const gltf::Primitive_drawcall& drawcall
			) const noexcept override;
		};

	  public:

		Gbuffer_gltf(const Gbuffer_gltf&) = delete;
		Gbuffer_gltf(Gbuffer_gltf&&) = default;
		Gbuffer_gltf& operator=(const Gbuffer_gltf&) = delete;
		Gbuffer_gltf& operator=(Gbuffer_gltf&&) = default;

		static std::expected<Gbuffer_gltf, util::Error> create(SDL_GPUDevice* device) noexcept;

		void render(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& gbuffer_pass,
			const drawdata::Gbuffer& drawdata
		) const noexcept;
	};
}