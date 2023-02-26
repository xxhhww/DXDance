#include "TextureLoader.h"
#include "Tools/StrUtil.h"
#include "Texture.h"
#include <assert.h>

namespace Core {
	void TextureLoader::Create(const std::string& path, Texture& texture) {
		if (!smInitialized) {
			CoInitialize(NULL);
			smInitialized = true;
		}

		std::string extension = Tool::StrUtil::GetFileExtension(path);
		
		if (extension == "png" || extension == "jpg") {
			CreateFromWICFile(path, texture);
		}
		else if (extension == "dds") {
			CreateFromDDSFile(path, texture);
		}
		else if (extension == "tga") {
			CreateFromTGAFile(path, texture);
		}
		else {
			assert(false);
		}
	}

	/*
	* 加载WICFile
	*/
	void TextureLoader::CreateFromWICFile(const std::string& path, Texture& texture) {
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
			texture
		);

		baseImage.Release();
	}

	/*
	* 加载DDSFile
	*/
	void TextureLoader::CreateFromDDSFile(const std::string& path, Texture& texture) {
	}

	/*
	* 加载TGAFile
	*/
	void TextureLoader::CreateFromTGAFile(const std::string& path, Texture& texture) {
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