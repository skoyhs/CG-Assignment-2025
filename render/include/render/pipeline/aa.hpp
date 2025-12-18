#pragma once

#include "graphics/aa/empty.hpp"
#include "graphics/aa/fxaa.hpp"
#include "graphics/aa/mlaa.hpp"
#include "graphics/aa/smaa.hpp"

#include "render/param.hpp"

namespace render::pipeline
{
	// Antialiasing Processor Module
	class Antialias
	{
		graphics::aa::Empty empty_processor;
		graphics::aa::FXAA fxaa_processor;
		graphics::aa::MLAA mlaa_processor;
		graphics::aa::SMAA smaa_processor;

		Antialias(
			graphics::aa::Empty empty_processor,
			graphics::aa::FXAA fxaa_processor,
			graphics::aa::MLAA mlaa_processor,
			graphics::aa::SMAA smaa_processor
		) noexcept :
			empty_processor(std::move(empty_processor)),
			fxaa_processor(std::move(fxaa_processor)),
			mlaa_processor(std::move(mlaa_processor)),
			smaa_processor(std::move(smaa_processor))
		{}

	  public:

		Antialias(const Antialias&) = delete;
		Antialias(Antialias&&) = default;
		Antialias& operator=(const Antialias&) = delete;
		Antialias& operator=(Antialias&&) = default;

		static std::expected<Antialias, util::Error> create(
			SDL_GPUDevice* device,
			SDL_GPUTextureFormat format
		) noexcept;

		std::expected<void, util::Error> run(
			SDL_GPUDevice* device,
			const gpu::Command_buffer& command_buffer,
			SDL_GPUTexture* source,
			SDL_GPUTexture* target,
			glm::u32vec2 size,
			Antialias_mode mode
		) noexcept;
	};
}