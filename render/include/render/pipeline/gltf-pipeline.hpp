#pragma once

#include "gltf/material.hpp"
#include "gltf/model.hpp"
#include "gltf/skin.hpp"
#include "gpu/command-buffer.hpp"
#include "gpu/render-pass.hpp"

namespace render::pipeline
{
	///
	/// @brief Interface for Gbuffer glTF pipelines
	///
	///
	class Gltf_pipeline
	{
	  public:

		virtual ~Gltf_pipeline() = default;

		///
		/// @brief Bind the pipeline to the command buffer and render pass
		///
		/// @param command_buffer Command buffer
		/// @param render_pass Render pass
		/// @param camera_matrix Camera matrix
		///
		virtual void bind(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			const glm::mat4& camera_matrix
		) const noexcept = 0;

		///
		/// @brief Set a material for the pipeline
		///
		/// @param command_buffer Command buffer
		/// @param render_pass Render pass
		/// @param material glTF Material
		///
		virtual void set_material(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			const gltf::Material_gpu& material
		) const noexcept = 0;

		///
		/// @brief Set a skinning resource for the pipeline
		///
		/// @param render_pass Render pass
		/// @param skinning_resource Skinning resource
		///
		virtual void set_skin(
			const gpu::Render_pass& render_pass,
			const gltf::Deferred_skinning_resource& skinning_resource
		) const noexcept = 0;

		///
		/// @brief Draw the primitive drawcall
		///
		/// @param command_buffer Command buffer
		/// @param render_pass Render pass
		/// @param drawcall Primitive drawcall
		///
		virtual void draw(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			const gltf::Primitive_drawcall& drawcall
		) const noexcept = 0;
	};
}