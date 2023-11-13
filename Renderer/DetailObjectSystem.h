#pragma once
#include "Renderer/HierarchyInstancedStaticMesh.h"


namespace Renderer {

	class RenderEngine;

	class DetailObjectSystem {
	public:
		struct CullClusterPassData {
			uint32_t clusterNodeBufferIndex;
			uint32_t visibleClusterNodeIndexBufferIndex;
			float pad1;
			float pad2;
		};

		struct CullStaticInstancePassData {
			uint32_t clusterNodeBufferIndex;
			uint32_t boundingBoxBufferIndex;
			uint32_t visibleClusterNodeIndexBufferIndex;
			uint32_t visibleLod0InstanceIndexBufferIndex;

			uint32_t visibleLod1InstanceIndexBufferIndex;
			uint32_t visibleLod2InstanceIndexBufferIndex;
			float pad1;
			float pad2;
		};

	public:
		DetailObjectSystem(RenderEngine* renderEngine);
		~DetailObjectSystem() = default;

		/*
		* ≥ı ºªØ
		*/
		void Initialize(RenderEngine* renderEngine);
		
		/*
		* ÃÌº”Pass
		*/
		void AddPass(RenderEngine* renderEngine);

	private:
		inline static uint32_t smCullClusterNodeThreadSizeInGroup = 8u;

		RenderEngine* mRenderEngine = nullptr;

		std::vector<std::unique_ptr<HierarchyInstancedStaticMesh>> mHierarchyInstancedStaticMeshs;
		std::vector<CullClusterPassData> mCullClusterPassDatas;
		std::vector<CullStaticInstancePassData> mCullStaticInstancePassDatas;
	};

}