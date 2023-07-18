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
			uint32_t rotateToCamera = 0u;	// Foliage����Ⱦʱ���Ƿ�ǿ�Ƴ��������
			float pad2;
			float pad3;
		};

		struct Placement {
			Math::Vector4 position;		// ���õ����������
			Math::Vector4 normal;		// ���õ��ϵĵ��η���
			Math::Matrix4 modelTrans;	// ���õ��ģ�ͱ任����
		};

		FoliagePassData foliagePassData;

		inline static uint64_t smMaxPlacementSize = 65536u;	// ���õ�ĸ��������ֵ		
		BufferWrap placementBuffer;							// Foliage�İ��õ�

		std::unique_ptr<Model> foliageQuadModel;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}