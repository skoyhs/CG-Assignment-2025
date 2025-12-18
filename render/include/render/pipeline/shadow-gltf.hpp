#pragma once

#include "gltf/material.hpp"
#include "gltf/model.hpp"
#include "gpu/command-buffer.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "gpu/render-pass.hpp"
#include "render/drawdata/shadow.hpp"
#include "render/pipeline/gltf-pipeline.hpp"
#include "render/target/shadow.hpp"

namespace render::pipeline
{
	class Shadow_gltf
	{
		// (Pipeline Mode, Rigged) -> Pipeline Instance
		std::map<std::pair<gltf::Pipeline_mode, bool>, std::unique_ptr<Gltf_pipeline>> pipelines;

		Shadow_gltf(
			std::map<std::pair<gltf::Pipeline_mode, bool>, std::unique_ptr<Gltf_pipeline>> pipelines
		) noexcept :
			pipelines(std::move(pipelines))
		{}

		struct alignas(64) Frag_param
		{
			float alpha_cutoff;
		};

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

		Shadow_gltf(const Shadow_gltf&) = delete;
		Shadow_gltf(Shadow_gltf&&) = default;
		Shadow_gltf& operator=(const Shadow_gltf&) = delete;
		Shadow_gltf& operator=(Shadow_gltf&&) = default;

		static std::expected<Shadow_gltf, util::Error> create(SDL_GPUDevice* device) noexcept;

		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			const target::Shadow& shadow_target,
			const drawdata::Shadow& drawdata
		) const noexcept;
	};
}