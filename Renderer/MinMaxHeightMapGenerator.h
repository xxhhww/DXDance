#pragma once
#include "DirectStorage/dstorage.h"
#include "DirectXTex/DirectXTex.h"

#include "Renderer/RenderEngine.h"

namespace GHL {
	class Fence;
}

namespace Renderer {

	/*
	* 一次性的纹理生成器
	*/
	class MinMaxHeightMapGenerator {
	public:

		void Initialize(const std::string& filename, RenderEngine* renderEngine);

		void Generate(CommandBuffer& commandBuffer, RenderContext& renderContext);

		void OnCompleted();

	private:
		Renderer::TextureDesc GetTextureDesc(const DirectX::TexMetadata& metadata);

	private:
		inline static uint32_t smMostDetailedPatchSize = 1280; // 整个地形需要最精细的Patch 1280 * 1280 才能填充完
		inline static uint32_t smThreadSizeInGroup1 = 8u; // 8 * 8 * 1 as a thread group
		inline static uint32_t smThreadSizeInGroup2 = 5u;
		std::unique_ptr<Texture> mHeightMap;
		std::unique_ptr<Texture> mMinMaxHeightMap;
	};

}