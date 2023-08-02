#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Model.h"

namespace Renderer {

	class RenderEngine;

	class FoliagePass {
	public:
		struct FoliagePassData {
			uint32_t placementBufferIndex;
			uint32_t heightMapIndex;
			uint32_t normalMapIndex;
			uint32_t rotateToCamera = 1u;	// Foliage����Ⱦʱ���Ƿ�ǿ�Ƴ��������
			Math::Vector2 worldMeterSize{ 5120u, 5120u };					// ������XZ�᷽��Ĵ�С(��)
			uint32_t heightScale{ 4096u };
			uint32_t placementSizePerAxis;
			uint32_t useFrustumCull = 1u;
			float    pad1;
			float    pad2;
			float    pad3;
		};

		struct Placement {
			// ������Ϣ
			Math::Vector3 position;		// ���õ������
			Math::Vector2 facing;		// ���õ�ĳ���
			uint32_t      type;			// ����������(Grass / Tree / Stone)
			uint32_t      lod;			// LOD����
			float         height;		// ?

			// ������Ϣ...
		};

		FoliagePassData foliagePassData;

		inline static uint32_t smThreadSizeInGroup = 8u;
		inline static uint32_t smPlacementSizePerAxis = 4096u;		// ÿ����İ��õ�ĸ���
		inline static uint32_t smMaxPlacementSize = 2048u * 2048u;	// ���õ�ĸ��������ֵ															// ���õ�����

		std::unique_ptr<Mesh>  foliageMesh;

		TextureWrap			   foliageAlbedoMap;
		TextureWrap            foliageNormalMap;
	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}