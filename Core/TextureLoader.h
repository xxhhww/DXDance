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
		static Texture* Create(const std::string& path);

		/*
		* 加载WICFile
		*/
		static Texture* CreateFromWICFile(const std::string& path);

		/*
		* 加载DDSFile
		*/
		static Texture* CreateFromDDSFile(const std::string& path);

		/*
		* 加载TGAFile
		*/
		static Texture* CreateFromTGAFile(const std::string& path);

		/*
		* 计算MipCounts
		*/
		static size_t CountMips(_In_ size_t width, _In_ size_t height);
	private:
		inline static bool smInitialized{ false };
	};
}