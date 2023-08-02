#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class RenderEngine;

	class PlaceObjectOnTerrainPass {
	public:
		struct Placement {
			// ������Ϣ
			Math::Vector3 position;		// ���õ������
			Math::Vector2 facing;		// ���õ�ĳ���
			uint32_t      type;			// ����������(Grass / Tree / Stone)
			uint32_t      lod;			// LOD����
			float         height;		// ?

			// ������Ϣ...
		};

		struct PlaceObjectOnTerrainPassData {
			uint32_t   grassPlacementBufferIndex0;	// Grass LOD 0
			uint32_t   grassPlacementBufferIndex1;	// Grass LOD 1
			uint32_t   heightMapIndex;
			uint32_t   normalMapIndex;

			Math::Vector2 worldMeterSize{ 5120u, 5120u };
			uint32_t   heightScale{ 4096u };
			uint32_t   placementSizePerAxis;

			uint32_t   useFrustumCull{ 1u };
			float  pad1;
			float  pad2;
			float  pad3;
		};

		PlaceObjectOnTerrainPassData placeObjectOnTerrainPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;
		inline static uint32_t smPlacementSizePerAxis = 4096u;		// ÿ����İ��õ�ĸ���
		inline static uint32_t smMaxPlacementSize = 2048u * 2048u;	// ���õ�ĸ��������ֵ

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}