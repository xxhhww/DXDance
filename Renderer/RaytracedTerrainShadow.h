#pragma once
#include <cstdint>

namespace Renderer {

	class RenderEngine;

	class RaytracedTerrainShadow {
	public:
		struct RaytracedTerrainShadowPassData {
		public:
			uint32_t terrainHeightMapIndex;
			uint32_t gBufferPositionEmissionMapIndex;
			uint32_t ssRaytracedTerrainShadowMapIndex;
			float pad1;
		};

	public:
		RaytracedTerrainShadow() = default;
		~RaytracedTerrainShadow() = default;

		/*
		* ��ʼ��
		*/
		void Initialize(RenderEngine* renderEngine);

		/*
		* ���Pass
		*/
		void AddPass(RenderEngine* renderEngine);

	private:
		inline static uint32_t smThreadSizeInGroup = 8u;

		RaytracedTerrainShadowPassData mRaytracedTerrainShadowPassData;
	};

}