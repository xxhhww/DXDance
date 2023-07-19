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
			uint32_t rotateToCamera = 1u;	// Foliage����Ⱦʱ���Ƿ�ǿ�Ƴ��������
			float pad1;
			float pad2;
		};

		struct Placement {
			Math::Vector4 position;			// ���õ����������
			Math::Vector4 normal;			// ���õ��ϵĵ��η���
			Math::Matrix4 modelTrans;		// ���õ��ģ�ͱ任����

			uint32_t      albedoMapIndex;	// ���õ��ϵ�ֲ��ķ�������ͼ
			float		  pad1;
			float		  pad2;
			float		  pad3;
		};

		FoliagePassData foliagePassData;

		inline static uint64_t smMaxPlacementSize = 65536u;	// ���õ�ĸ��������ֵ	
		std::vector<Placement> placements;					// ���õ�����

		std::unique_ptr<Model> foliageQuadModel;
		Mesh*				   foliageQuadMesh = nullptr;

		TextureWrap			   foliageAlbedoMap;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}