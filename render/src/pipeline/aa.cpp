#include "render/pipeline/aa.hpp"
#include "render.hpp"

namespace render::pipeline
{
	std::expected<Antialias, util::Error> Antialias::create(
		SDL_GPUDevice* device,
		SDL_GPUTextureFormat format
	) noexcept
	{
		auto empty_processor = graphics::aa::Empty{};
		auto fxaa_processor = graphics::aa::FXAA::create(device, format);
		auto mlaa_processor = graphics::aa::MLAA::create(device, format);
		auto smaa_processor = graphics::aa::SMAA::create(device, format);

		if (!fxaa_processor) return fxaa_processor.error().forward("Create FXAA processor failed");
		if (!mlaa_processor) return mlaa_processor.error().forward("Create MLAA processor failed");
		if (!smaa_processor) return smaa_processor.error().forward("Create SMAA processor failed");

		return Antialias{
			std::move(empty_processor),
			std::move(*fxaa_processor),
			std::move(*mlaa_processor),
			std::move(*smaa_processor)
		};
	}

	std::expected<void, util::Error> Antialias::run(
		SDL_GPUDevice* device,
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* source,
		SDL_GPUTexture* target,
		glm::u32vec2 size,
		Antialias_mode mode
	) noexcept
	{
		command_buffer.push_debug_group("Antialiasing Pass");
		auto result = [&] -> std::expected<void, util::Error> {
			switch (mode)
			{
			case Antialias_mode::None:
				return empty_processor.run_antialiasing(device, command_buffer, source, target, size);
			case Antialias_mode::FXAA:
				return fxaa_processor.run_antialiasing(device, command_buffer, source, target, size);
			case Antialias_mode::MLAA:
				return mlaa_processor.run_antialiasing(device, command_buffer, source, target, size);
			case Antialias_mode::SMAA:
				return smaa_processor.run_antialiasing(device, command_buffer, source, target, size);
			default:
				return util::Error("Unknown antialiasing mode");
			}
		}();
		command_buffer.pop_debug_group();

		return result;
	}
}