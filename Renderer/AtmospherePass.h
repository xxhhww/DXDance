#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RenderEngine;

	class AtmospherePass {
	public:
		struct AtmosphereParameter {
			float SeaLevel = 0.0f;
			float PlanetRadius = 6360000.0f;
			float AtmosphereHeight = 60000.0f;
			float SunLightIntensity = 15.0f;

			Math::Vector3 SunLightColor = Math::Vector3{ 1.0f, 1.0f, 1.0f };
			float SunDiskAngle = 1.0f;

			float RayleighScatteringScale = 1.0f;
			float RayleighScatteringScalarHeight = 8000.0f;
			float MieScatteringScale = 1.0f;
			float MieAnisotropy = 0.8f;

			float MieScatteringScalarHeight = 1200.0f;
			float OzoneAbsorptionScale = 1.0f;
			float OzoneLevelCenterHeight = 25000.0f;
			float OzoneLevelWidth = 15000.0f;
		};

		struct AtmosphereBuilderData {
			AtmosphereParameter parameter;
			uint32_t transmittanceLutIndex;
			Math::Vector2 transmittanceLutSize;
			float pad1;

			uint32_t multiScatteringLutIndex;
			Math::Vector2 multiScatteringLutSize;
			float pad2;

			uint32_t skyViewLutIndex;
			Math::Vector2 skyViewLutSize;
			float pad3;

			uint32_t aerialPerspectiveLutIndex;
			Math::Vector2 aerialPerspectiveLutSize;
			float pad4;

			Math::Vector3 aerialPerspectiveVoxelSize = Math::Vector3{ 32.0f, 32.0f, 32.0f };
			float aerialPerspectiveDistance = 32000.0f;
		};

		struct AtmosphereRendererData {
			AtmosphereParameter parameter;

			uint32_t transmittanceLutIndex;
			uint32_t multiScatteringLutIndex;
			uint32_t skyViewLutIndex;
			uint32_t aerialPerspectiveLutIndex;
		};

	public:
		inline static uint32_t smThreadSizeInGroup = 8u;
		AtmosphereBuilderData atmosphereBuilderData;
		AtmosphereRendererData atmosphereRendererData;

		std::unique_ptr<Renderer::Mesh> cubeMesh;

	public:
		void AddPass(RenderGraph& renderGraph);

		void Initialize(RenderEngine* renderEngine);
	};

}