#pragma once

#include "gpu/buffer.hpp"
#include <cstdint>
#include <expected>

namespace render::target
{
	class Auto_exposure
	{
	  public:

		static constexpr uint32_t bin_count = 256;

		struct Frame
		{
			SDL_GPUBuffer* bin_buffer;
			SDL_GPUBuffer* previous_result_buffer;
			SDL_GPUBuffer* result_buffer;
		};

		///
		/// @brief Create auto exposure resource
		///
		/// @return Auto exposure resource on success, or an error on failure
		///
		static std::expected<Auto_exposure, util::Error> create(SDL_GPUDevice* device) noexcept;

		///
		/// @brief Cycle internal resource. Called exactly once every frame.
		///
		///
		void cycle() noexcept;

		///
		/// @brief Get resource set for current frame.
		///
		/// @return Frame containing buffers for the current frame.
		///
		Frame get_current_frame() const noexcept;

	  private:

		struct Bin_buffer_struct
		{
			uint32_t bins[bin_count];
			uint32_t sum;
		};

		struct Result_buffer_struct
		{
			float avg_brightness;
			float exposure_mult;
		};

		gpu::Buffer bin_buffer;
		std::vector<gpu::Buffer> result_buffer;
		uint32_t current_idx = 0;

		Auto_exposure(gpu::Buffer bin_buffer, std::vector<gpu::Buffer> result_buffer) noexcept :
			bin_buffer(std::move(bin_buffer)),
			result_buffer(std::move(result_buffer))
		{}

	  public:

		Auto_exposure(const Auto_exposure&) = delete;
		Auto_exposure(Auto_exposure&&) = default;
		Auto_exposure& operator=(const Auto_exposure&) = delete;
		Auto_exposure& operator=(Auto_exposure&&) = default;
		~Auto_exposure() = default;
	};
}