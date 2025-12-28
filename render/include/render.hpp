#pragma once

#include <expected>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include "gltf/model.hpp"
#include "render/drawdata/light.hpp"
#include "render/param.hpp"
#include "render/pipeline.hpp"
#include "render/target.hpp"

namespace render
{
	struct Drawdata
	{
		std::span<const gltf::Drawdata> models;
		std::span<const drawdata::Light> lights;
	};

	class Renderer
	{
	  public:

		static std::expected<Renderer, util::Error> create(const backend::SDL_context& sdl_context) noexcept;

		std::expected<void, util::Error> render(
			const backend::SDL_context& sdl_context,
			Drawdata drawdata,
			const Params& params
		) noexcept;

	  private:

		Pipeline pipeline;
		Target target;

		graphics::Buffer_pool buffer_pool;
		graphics::Transfer_buffer_pool transfer_buffer_pool;

		std::expected<std::tuple<drawdata::Gbuffer, drawdata::Shadow>, util::Error> prepare_drawdata(
			std::span<const gltf::Drawdata> drawdata_list,
			const Params& params
		) noexcept;

		std::expected<void, util::Error> copy_resources(
			const gpu::Command_buffer& command_buffer,
			std::span<const gltf::Drawdata> drawdata_list
		) const noexcept;

		std::expected<void, util::Error> render_gbuffer(
			const gpu::Command_buffer& command_buffer,
			const drawdata::Gbuffer& gbuffer_drawdata,
			const Params& params
		) const noexcept;

		std::expected<void, util::Error> render_ao(
			const gpu::Command_buffer& command_buffer,
			const Params& params
		) const noexcept;

		std::expected<void, util::Error> render_lighting(
			const gpu::Command_buffer& command_buffer,
			const drawdata::Shadow& shadow_drawdata,
			const Params& params,
			glm::u32vec2 swapchain_size
		) const noexcept;

		std::expected<void, util::Error> render_lights(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer_target,
			const target::Light_buffer& light_buffer_target,
			std::span<const drawdata::Light> lights,
			const Params& params,
			glm::u32vec2 swapchain_size
		) const noexcept;

		std::expected<void, util::Error> render_ssgi(
			const gpu::Command_buffer& command_buffer,
			const drawdata::Gbuffer& gbuffer_drawdata,
			const Params& params,
			glm::u32vec2 swapchain_size
		) const noexcept;

		std::expected<void, util::Error> compute_auto_exposure(
			const gpu::Command_buffer& command_buffer,
			glm::u32vec2 swapchain_size
		) const noexcept;

		std::expected<void, util::Error> render_bloom(
			const gpu::Command_buffer& command_buffer,
			const Params& params,
			glm::u32vec2 swapchain_size
		) const noexcept;

		std::expected<void, util::Error> render_composite(
			SDL_GPUDevice* device,
			const gpu::Command_buffer& command_buffer,
			const Params& params,
			glm::u32vec2 swapchain_size,
			SDL_GPUTexture* swapchain
		) noexcept;

		std::expected<void, util::Error> render_imgui(
			const gpu::Command_buffer& command_buffer,
			SDL_GPUTexture* swapchain
		) const noexcept;

		Renderer(
			Pipeline pipeline,
			Target target,
			graphics::Buffer_pool buffer_pool,
			graphics::Transfer_buffer_pool transfer_buffer_pool
		) :
			pipeline(std::move(pipeline)),
			target(std::move(target)),
			buffer_pool(std::move(buffer_pool)),
			transfer_buffer_pool(std::move(transfer_buffer_pool))
		{}

	  public:

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) = default;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = default;
	};
}