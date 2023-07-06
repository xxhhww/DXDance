#pragma once
#include "DirectStorage/dstorage.h"
#include "DirectXTex/DirectXTex.h"

#include "Renderer/RenderEngine.h"

namespace GHL {
	class Fence;
}

namespace Renderer {

	/*
	* 生成云噪声纹理的离线任务
	*/
	class GenerateCloudNoiseTask {
	public:
		struct ShapeNoiseGeneratorPassData {
			uint32_t shapeNoiseMapIndex;
			uint32_t shapeNoiseMapWidth;
			uint32_t shapeNoiseMapHeight;
			uint32_t shapeNoiseMapDepth;
		};

		struct DetailNoiseGeneratorPassData {
			uint32_t detailNoiseMapIndex;
			uint32_t detailNoiseMapWidth;
			uint32_t detailNoiseMapHeight;
			uint32_t detailNoiseMapDepth;
		};

	public:
		void Initialize(RenderEngine* renderEngine);

		void Generate(CommandBuffer& commandBuffer, RenderContext& renderContext);

		void OnCompleted();

	private:
		inline static uint32_t smThreadSizeInGroup1 = 8u; // 8 * 8 * 8 as a thread group
		inline static uint32_t smThreadSizeInGroup2 = 8u;

		GHL::Device* mDevice{ nullptr };

		TextureWrap mShapeNoiseMap;
		TextureWrap mDetailNoiseMap;

		BufferWrap mShapeNoiseMapReadback;
		BufferWrap mDetailNoiseMapReadback;

		ShapeNoiseGeneratorPassData  shapeNoiseGeneratorPassData;
		DetailNoiseGeneratorPassData detailNoiseGeneratorPassData;
	};

}