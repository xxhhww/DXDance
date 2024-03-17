#pragma once
#include <string>

namespace OfflineTask {

	/*
	* 对高度图进行倒置和分割操作
	*/
	class TextureProcessor {
	public:
		/*
		* 做纹理填充
		*/
		static void Padding(const std::string& filename, const std::string& dirname, uint32_t paddingSize);

		/*
		* 将图像进行分割，并将分割后的结果填入到目标目录下
		*/
		static void Split(const std::string& filename, const std::string& dirname, uint32_t subSize, uint32_t step, uint32_t startNameIndex);

		/*
		* 图像GenerateLodMap
		*/
		static void GenerateTerrainMipMap(const std::string& filename, const std::string& dirname, uint32_t mipIndex, uint32_t targetWidth, uint32_t targetHeight);

		/*
		* 将dirname下的所有图像合并为TextureAtlasFile
		*/
		static void GenerateTextureAtlasFile1(const std::string& srcDirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& dstFilename);

		/*
		* 将纹理或者纹理数组转换为TextureAtlasFile
		*/
		static void GenerateTextureAtlasFile2(const std::string& srcFilename, const std::string& dstFilename);

		/*
		* 将dirname下的所有SplatMap进行变换合并为一个SplatMap
		*/
		static void ConvertSplatMap(const std::string& dirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& targetFilename);
	
		/*
		* 解算由ConvertSplatMap方法产生的SplatMap，从而验证其正确性
		*/
		static void ResolveConvertedSplatMap(const std::string& filename, const std::string& targetFilename);

		/*
		* 将dirname下的所有图像合并为一个TextureArray，并生成MipChain，最后一级的MipChain上的每一个子资源的大小必需大于等于64kb
		*/
		static void GenerateTextureArray(const std::string& srcDirname, const std::string& dstFilename);

		/*
		* 生成一个随机颜色图
		*/
		static void GenerateRandomAlbedoMap(const std::string& dstFilename, uint32_t width, uint32_t height);

		/*
		* 生成每一个地形节点的描述
		*/
		static void GenerateTerrainQuadNodeDescriptors(const std::string& heightMapFilename, const std::string& dstDirname);

		/*
		* 根据地形高度图生成二进制数据
		*/
		static void GenerateHeightBinData(const std::string& heightMapFilename, const std::string& binDataFilename);
	};

}
