#include "TextureLoader.h"
#include "Tools/StrUtil.h"
#include "Texture.h"
#include <assert.h>

namespace Core {
	Texture* TextureLoader::Create(const std::string& path) {
		if (!smInitialized) {
			CoInitialize(NULL);
			smInitialized = true;
		}

		std::string extension = Tool::StrUtil::GetFileExtension(path);
		
		if (path == "png") {
			return CreateFromWICFile(path);
		}
		else if (path == "dds") {
			return CreateFromDDSFile(path);
		}
		else if (path == "tga") {
			return CreateFromTGAFile(path);
		}
		else {
			assert(false);
		}
		return nullptr;
	}

	/*
	* 加载WICFile
	*/
	Texture* TextureLoader::CreateFromWICFile(const std::string& path) {
		// 获取资产名称
		std::string name = Tool::StrUtil::RemoveBasePath(path);
		
		Texture* texture = new Texture(name);

		DirectX::ScratchImage baseImage;
		DirectX::LoadFromWICFile(
			Tool::StrUtil::UTF8ToWString(path).c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			baseImage
		);

		// Compute MipMaps
		uint32_t mipMaps = CountMips(baseImage.GetMetadata().width, baseImage.GetMetadata().height);
		
		// GenMipMaps
		DirectX::GenerateMipMaps(
			baseImage.GetImages(),
			baseImage.GetImageCount(),
			baseImage.GetMetadata(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			mipMaps,
			*texture
		);

		return texture;
	}

	/*
	* 加载DDSFile
	*/
	Texture* TextureLoader::CreateFromDDSFile(const std::string& path) {
		return nullptr;
	}

	/*
	* 加载TGAFile
	*/
	Texture* TextureLoader::CreateFromTGAFile(const std::string& path) {
		return nullptr;
	}

	/*
	* 计算MipCounts
	*/
	size_t TextureLoader::CountMips(_In_ size_t width, _In_ size_t height) {
		size_t mipLevels = 1;

		while (height > 1 || width > 1) {
			if (height > 1)
				height >>= 1;

			if (width > 1)
				width >>= 1;

			++mipLevels;
		}

		return mipLevels;
	}
}