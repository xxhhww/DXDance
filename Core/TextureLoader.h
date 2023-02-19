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
		* ͨ������·������������
		*/
		static Texture* Create(const std::string& path);

		/*
		* ����WICFile
		*/
		static Texture* CreateFromWICFile(const std::string& path);

		/*
		* ����DDSFile
		*/
		static Texture* CreateFromDDSFile(const std::string& path);

		/*
		* ����TGAFile
		*/
		static Texture* CreateFromTGAFile(const std::string& path);

		/*
		* ����MipCounts
		*/
		static size_t CountMips(_In_ size_t width, _In_ size_t height);
	private:
		inline static bool smInitialized{ false };
	};
}