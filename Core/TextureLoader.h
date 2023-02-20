#pragma once
#include <string>
#include <wincodec.h>
#include <wrl.h>

using namespace Microsoft;

namespace Core {
	class Texture;

	class TextureLoader {
	public:
		/*
		* Disabled constructor
		*/
		TextureLoader() = delete;

		/*
		* 通过绝对路径来创建纹理
		*/
		static void Create(const std::string& path, Texture& texture);

		/*
		* 加载WICFile
		*/
		static void CreateFromWICFile(const std::string& path, Texture& texture);

		/*
		* 加载DDSFile
		*/
		static void CreateFromDDSFile(const std::string& path, Texture& texture);

		/*
		* 加载TGAFile
		*/
		static void CreateFromTGAFile(const std::string& path, Texture& texture);

		/*
		* 计算MipCounts
		*/
		static size_t CountMips(_In_ size_t width, _In_ size_t height);
	private:
		inline static bool smInitialized{ false };
	};
}