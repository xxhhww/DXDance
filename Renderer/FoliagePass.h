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
			uint32_t rotateToCamera = 0u;	// Foliage在渲染时，是否强制朝向摄像机
			float pad2;
			float pad3;
		};

		struct Placement {
			Math::Vector4 position;		// 安置点的世界坐标
			Math::Vector4 normal;		// 安置点上的地形法线
			Math::Matrix4 modelTrans;	// 安置点的模型变换矩阵
		};

		FoliagePassData foliagePassData;

		inline static uint64_t smMaxPlacementSize = 65536u;	// 安置点的个数的最大值		
		BufferWrap placementBuffer;							// Foliage的安置点

		std::unique_ptr<Model> foliageQuadModel;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}