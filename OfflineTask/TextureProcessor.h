#pragma once
#include <string>

namespace OfflineTask {

	/*
	* 对高度图进行倒置和分割操作
	*/
	class TextureProcessor {
	public:
		static void Padding(const std::string& filename, const std::string& dirname, uint32_t paddingSize);

		/*
		* 将图像进行分割，并将分割后的结果填入到目标目录下
		*/
		static void Split(const std::string& filename, const std::string& dirname, uint32_t subSize, uint32_t step, uint32_t startNameIndex);

		static void Resize(const std::string& filename, const std::string& dirname, uint32_t targetWidth, uint32_t targetHeight);

		/*
		* 将dirname下的所有图像合并为TextureAtlasFile
		*/
		static void MergeTextureAtlasFile(const std::string& dirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& targetFilename);

	};

}
