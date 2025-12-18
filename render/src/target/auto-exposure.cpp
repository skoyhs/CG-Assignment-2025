#include "render/target/auto-exposure.hpp"
#include "graphics/util/quick-create.hpp"
#include "util/as-byte.hpp"

#include <ranges>

namespace render::target
{
	std::expected<Auto_exposure, util::Error> Auto_exposure::create(SDL_GPUDevice* device) noexcept
	{
		auto bin_buffer = gpu::Buffer::create(
			device,
			{.graphic_storage_read = true, .compute_storage_read = true, .compute_storage_write = true},
			sizeof(Bin_buffer_struct),
			"Auto Exposure Bin Buffer"
		);
		if (!bin_buffer) return bin_buffer.error().forward("Create auto exposure bin buffer failed");

		const auto clear_value = Result_buffer_struct{.avg_brightness = 1.0f, .exposure_mult = 1.0f};

		std::vector<gpu::Buffer> result_buffer;
		for (auto idx : std::views::iota(0u, 3u))
		{
			auto buffer = graphics::create_buffer_from_data(
				device,
				{.graphic_storage_read = true, .compute_storage_read = true, .compute_storage_write = true},
				util::as_bytes(clear_value),
				std::format("Auto Exposure Result Buffer [Index {}]", idx)
			);
			if (!buffer) return buffer.error().forward("Create auto exposure result buffer failed");
			result_buffer.push_back(std::move(*buffer));
		}

		return Auto_exposure{std::move(*bin_buffer), std::move(result_buffer)};
	}

	void Auto_exposure::cycle() noexcept
	{
		current_idx = (current_idx + 1) % result_buffer.size();
	}

	Auto_exposure::Frame Auto_exposure::get_current_frame() const noexcept
	{
		const auto prev_idx = (current_idx + result_buffer.size() - 1) % result_buffer.size();

		return Frame{
			.bin_buffer = bin_buffer,
			.previous_result_buffer = result_buffer[prev_idx],
			.result_buffer = result_buffer[current_idx]
		};
	}
}