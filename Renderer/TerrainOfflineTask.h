#pragma once
#include "DirectStorage/dstorage.h"
#include "DirectXTex/DirectXTex.h"

#include "Renderer/RenderEngine.h"

namespace GHL {
	class Fence;
}

namespace Renderer {

	/*
	* ������Ⱦ����������
	*/
	class TerrainOfflineTask {
	public:
		void Initialize(const std::string& filename, RenderEngine* renderEngine);

		void Generate(CommandBuffer& commandBuffer, RenderContext& renderContext);

		void OnCompleted();

	private:
		Renderer::TextureDesc GetTextureDesc(const DirectX::TexMetadata& metadata);

		DirectX::TexMetadata GetTexMetadata(const Renderer::TextureDesc& textureDesc);

	private:
		inline static uint32_t smMostDetailedPatchSize = 640; // ����������Ҫ�ϸ��Patch 640 * 640 ���������
		inline static uint32_t smThreadSizeInGroup1 = 8u; // 8 * 8 * 1 as a thread group
		inline static uint32_t smThreadSizeInGroup2 = 5u;
		inline static uint32_t smThreadSizeInGroup3 = 8u;

		GHL::Device* mDevice{ nullptr };

		std::unique_ptr<Texture> mHeightMap;
		std::unique_ptr<Texture> mMinMaxHeightMap;
		std::unique_ptr<Texture> mNormalMap;
		std::unique_ptr<Buffer> mMinMaxHeightMapReadback;
		std::unique_ptr<Buffer> mNormalMapReadback;
	};

}