#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/render-pass.hpp"
#include "render/target/ao.hpp"
#include "render/target/gbuffer.hpp"
#include "render/target/light.hpp"
#include "render/target/shadow.hpp"

namespace render
{
	///
	/// @brief Acquire and begin G-buffer Render Pass
	/// @note This doesn't resize the render targets. Resize before use.
	///	@details
	/// #### Layout
	/// Color Attachments:
	/// 1. (`location=0`) Albedo Texture
	/// 2. (`location=1`) Lighting Info Texture
	/// 3. (`location=2`) Light Buffer Texture
	///
	/// Depth-Stencil Attachment:
	/// 1. Depth Texture, from Gbuffer
	///
	/// @param command_buffer Command Buffer
	/// @param gbuffer G-buffer Target
	/// @param light_buffer Light Buffer Target
	/// @return Acquired Render Pass
	///
	std::expected<gpu::Render_pass, util::Error> acquire_gbuffer_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer,
		const target::Light_buffer& light_buffer
	) noexcept;

	///
	/// @brief Acquire and begin Lighting Render Pass
	/// @note This doesn't resize the render targets. Resize before use.
	/// @details
	/// #### Layout
	/// Color Attachments:
	/// 1. (`location=0`) Light Buffer Texture
	///
	/// @param command_buffer Command Buffer
	/// @param light_buffer Light Buffer Target
	/// @return Acquired Render Pass
	///
	std::expected<gpu::Render_pass, util::Error> acquire_lighting_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::Gbuffer& gbuffer
	) noexcept;

	///
	/// @brief Acquire and begin Shadow Render Pass
	/// @note This doesn't resize the render targets. Resize before use.
	/// @details
	/// #### Layout
	/// Depth-Stencil Attachment:
	/// 1. Depth Texture, from Shadow Target
	///
	/// @param command_buffer Command Buffer
	/// @param shadow_target Shadow Target
	/// @return Acquired Render Pass
	///
	std::expected<gpu::Render_pass, util::Error> acquire_shadow_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Shadow& shadow_target,
		size_t level
	) noexcept;

	///
	/// @brief Acquire and begin Ambient Occlusion Render Pass
	/// @note This doesn't resize the render targets. Resize before use.
	/// @details
	/// #### Layout
	/// Color Attachments:
	/// 1. (`location=0`) AO Texture
	///
	/// @param command_buffer Command Buffer
	/// @param ao_target AO Target
	/// @return Acquired Render Pass
	///
	std::expected<gpu::Render_pass, util::Error> acquire_ao_pass(
		const gpu::Command_buffer& command_buffer,
		const target::AO& ao_target
	) noexcept;

	///
	/// @brief Acquire and begin Swapchain Render Pass
	/// @note This doesn't resize the render targets. Resize before use.
	/// @details
	/// #### Layout
	/// Color Attachments:
	/// 1. (`location=0`) Swapchain Texture
	///
	/// @param command_buffer Command Buffer
	/// @param swapchain_texture Swapchain Texture
	/// @return Acquired Render Pass
	///
	std::expected<gpu::Render_pass, util::Error> acquire_swapchain_pass(
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* swapchain_texture,
		bool clear
	) noexcept;
}