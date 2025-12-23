#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"

#include "render/target/auto-exposure.hpp"
#include "render/target/bloom.hpp"
#include "render/target/light.hpp"

#include <glm/glm.hpp>

namespace render::pipeline
{
	class Tonemapping
	{
	  public:

		struct Param
		{
			float exposure = 1.0f;
			float bloom_strength = 1.0f;
			bool use_bloom_mask;
		};

		static std::expected<Tonemapping, util::Error> create(
			SDL_GPUDevice* device,
			SDL_GPUTextureFormat target_format
		) noexcept;

		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			const target::Light_buffer& light_buffer,
			const target::Auto_exposure& auto_exposure,
			const target::Bloom& bloom,
			SDL_GPUTexture* target_texture,
			const Param& param
		) const noexcept;

	  private:

		graphics::Fullscreen_pass<true> fullscreen_pass;
		gpu::Sampler nearest_sampler;
		gpu::Sampler linear_sampler;
		gpu::Texture bloom_mask_dummy;
		gpu::Texture bloom_mask;

		Tonemapping(
			graphics::Fullscreen_pass<true> fullscreen_pass,
			gpu::Sampler nearest_sampler,
			gpu::Sampler linear_sampler,
			gpu::Texture bloom_mask_dummy,
			gpu::Texture bloom_mask
		) :
			fullscreen_pass(std::move(fullscreen_pass)),
			nearest_sampler(std::move(nearest_sampler)),
			linear_sampler(std::move(linear_sampler)),
			bloom_mask_dummy(std::move(bloom_mask_dummy)),
			bloom_mask(std::move(bloom_mask))
		{}

	  public:

		Tonemapping(const Tonemapping&) = delete;
		Tonemapping(Tonemapping&&) = default;
		Tonemapping& operator=(const Tonemapping&) = delete;
		Tonemapping& operator=(Tonemapping&&) = default;
	};
}