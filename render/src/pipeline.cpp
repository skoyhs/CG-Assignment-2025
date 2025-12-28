#include "render/pipeline.hpp"
#include "render/target/composite.hpp"

namespace render
{
	std::expected<Pipeline, util::Error> Pipeline::create(const backend::SDL_context& context) noexcept
	{
		auto aa_module = pipeline::Antialias::create(context.device, context.get_swapchain_texture_format());
		if (!aa_module) return aa_module.error().forward("Create antialias module failed");

		auto directional_light_pipeline = pipeline::Directional_light::create(context.device);
		if (!directional_light_pipeline)
			return directional_light_pipeline.error().forward("Create directional light pipeline failed");

		auto ambient_light_pipeline = pipeline::Ambient_light::create(context.device);
		if (!ambient_light_pipeline)
			return ambient_light_pipeline.error().forward("Create environment light pipeline failed");

		auto tonemapping_pipeline =
			pipeline::Tonemapping::create(context.device, target::Composite::composite_format.format);
		if (!tonemapping_pipeline)
			return tonemapping_pipeline.error().forward("Create tonemapping pipeline failed");

		auto ao_pipeline = pipeline::AO::create(context.device);
		if (!ao_pipeline) return ao_pipeline.error().forward("Create AO pipeline failed");

		auto sky_pipeline = pipeline::Sky_preetham::create(context.device);
		if (!sky_pipeline) return sky_pipeline.error().forward("Create sky pipeline failed");

		auto auto_exposure_pipeline = pipeline::Auto_exposure::create(context.device);
		if (!auto_exposure_pipeline)
			return auto_exposure_pipeline.error().forward("Create auto exposure pipeline failed");

		auto bloom_pipeline = pipeline::Bloom::create(context.device);
		if (!bloom_pipeline) return bloom_pipeline.error().forward("Create bloom pipeline failed");

		auto gbuffer_gltf = pipeline::Gbuffer_gltf::create(context.device);
		if (!gbuffer_gltf) return gbuffer_gltf.error().forward("Create Gbuffer Gltf pipeline failed");

		auto shadow_gltf = pipeline::Shadow_gltf::create(context.device);
		if (!shadow_gltf) return shadow_gltf.error().forward("Create Shadow Gltf pipeline failed");

		auto hiz_pipeline = pipeline::Hiz_generator::create(context.device);
		if (!hiz_pipeline) return hiz_pipeline.error().forward("Create HiZ pipeline failed");

		auto ssgi_pipeline = pipeline::SSGI::create(context.device);
		if (!ssgi_pipeline) return ssgi_pipeline.error().forward("Create SSGI pipeline failed");

		auto point_light_pipeline = pipeline::Light::create(context.device);
		if (!point_light_pipeline)
			return point_light_pipeline.error().forward("Create Point Light pipeline failed");

		auto depth_to_color_copier =
			graphics::Renderpass_copy::create(context.device, 1, target::Gbuffer::depth_value_format);
		if (!depth_to_color_copier)
			return depth_to_color_copier.error().forward("Create depth to color copier failed");

		return Pipeline{
			.aa_module = std::move(*aa_module),
			.ambient_light = std::move(*ambient_light_pipeline),
			.ao = std::move(*ao_pipeline),
			.auto_exposure = std::move(*auto_exposure_pipeline),
			.bloom = std::move(*bloom_pipeline),
			.directional_light = std::move(*directional_light_pipeline),
			.gbuffer_gltf = std::move(*gbuffer_gltf),
			.hiz_generator = std::move(*hiz_pipeline),
			.shadow_gltf = std::move(*shadow_gltf),
			.sky_preetham = std::move(*sky_pipeline),
			.ssgi = std::move(*ssgi_pipeline),
			.tonemapping = std::move(*tonemapping_pipeline),
			.point_light = std::move(*point_light_pipeline),
			.depth_to_color_copier = std::move(*depth_to_color_copier)
		};
	}
}