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
		static void Create(const std::string& path, Texture& texture);

		/*
		* ����WICFile
		*/
		static void CreateFromWICFile(const std::string& path, Texture& texture);

		/*
		* ����DDSFile
		*/
		static void CreateFromDDSFile(const std::string& path, Texture& texture);

		/*
		* ����TGAFile
		*/
		static void CreateFromTGAFile(const std::string& path, Texture& texture);

		/*
		* ����MipCounts
		*/
		static size_t CountMips(_In_ size_t width, _In_ size_t height);
	private:
		inline static bool smInitialized{ false };
	};
}