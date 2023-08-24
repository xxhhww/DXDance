#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RenderEngine;

	class OceanPass {
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

		struct WaterParameter {
            Math::Vector4 waterSurfaceColor = Math::Vector4(0.465f, 0.797f, 0.991f, 1.0f);
            Math::Vector4 waterRefractionColor = Math::Vector4(0.003f, 0.599f, 0.812f, 1.0f);
            Math::Vector4 ssrSettings = Math::Vector4(0.5f, 20.0f, 10.0f, 20.0f);
            Math::Vector4 normalMapScroll = Math::Vector4(1.0f, 0.0f, 0.0f, 1.0f);

            Math::Vector2 normalMapScrollSpeed = Math::Vector2(0.01f, 0.01f);
            float  refractionDistortionFactor = 0.04f;
            float  refractionHeightFactor = 2.5f;

            float  refractionDistanceFactor = 15.0f;
            float  depthSofteningDistance = 0.5f;
            float  foamHeightStart = 0.8f;
            float  foamFadeDistance = 0.4f;

            float  foamTiling = 2.0f;
            float  foamAngleExponent = 80.0f;
            float  roughness = 0.08f;
            float  reflectance = 0.55f;

            float  specIntensity = 125.0f;
            float  foamBrightness = 4.0f;
            float  tessellationFactor = 7.0f;
            float  dampeningFactor = 5.0f;

            uint32_t   waterNormalMap1Index;
            uint32_t   waterNormalMap2Index;
            uint32_t   waterFoamMapIndex;
            uint32_t   waterNoiseMapIndex;
		};

		struct OceanPassData {
            WaterParameter waterParameter;
            // AtmosphereParameter atmosphereParameter;

            uint32_t transmittanceLutIndex;
            uint32_t skyViewLutIndex;
            float pad1;
            float pad2;
		};

	public:
		OceanPassData oceanPassData;

		std::unique_ptr<Renderer::Mesh> gridMesh;
        
        TextureWrap waterNormalMap1;
        TextureWrap waterNormalMap2;
        TextureWrap waterFoamMap;
        TextureWrap waterNoiseMap;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	};

}