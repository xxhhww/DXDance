#pragma once
#include "Renderer/ReTextureFileFormat.h"
#include "Renderer/ResourceFormat.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"

#include "GHL/DirectStorageFile.h"

namespace GHL {
	class Device;
}

namespace Renderer {

	class TerrainRenderer;

	/*
	* ���������Ⱦ��ʹ�õ��������飬�����������Ա�����ʽ����
	*/
	class TerrainTextureArray {
	public:
		TerrainTextureArray(TerrainRenderer* renderer, const std::string& filepath);

		inline ~TerrainTextureArray() = default;

		inline const auto& GetReTextureFileFormat() const { return mReTextureFileFormat; }
		inline const auto& GetResourceFormat() const { return mResourceFormat; }

		inline auto* GetDStorageFile() const { return mDStorageFile.get(); }
		inline auto& GetTextureArray() const { return mTextureArray; }

	private:
		TerrainRenderer* mRenderer{ nullptr };
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		ReTextureFileFormat mReTextureFileFormat;
		ResourceFormat mResourceFormat;
		std::unique_ptr<GHL::DirectStorageFile> mDStorageFile;

		TextureWrap mTextureArray;
	};

}