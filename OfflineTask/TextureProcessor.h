#pragma once
#include <string>

namespace OfflineTask {

	/*
	* �Ը߶�ͼ���е��úͷָ����
	*/
	class TextureProcessor {
	public:
		/*
		* ���������
		*/
		static void Padding(const std::string& filename, const std::string& dirname, uint32_t paddingSize);

		/*
		* ��ͼ����зָ�����ָ��Ľ�����뵽Ŀ��Ŀ¼��
		*/
		static void Split(const std::string& filename, const std::string& dirname, uint32_t subSize, uint32_t step, uint32_t startNameIndex);

		/*
		* ͼ��GenerateLodMap
		*/
		static void GenerateTerrainMipMap(const std::string& filename, const std::string& dirname, uint32_t mipIndex, uint32_t targetWidth, uint32_t targetHeight);

		/*
		* ��dirname�µ�����ͼ��ϲ�ΪTextureAtlasFile
		*/
		static void GenerateTextureAtlasFile1(const std::string& srcDirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& dstFilename);

		/*
		* �����������������ת��ΪTextureAtlasFile
		*/
		static void GenerateTextureAtlasFile2(const std::string& srcFilename, const std::string& dstFilename);

		/*
		* ��dirname�µ�����SplatMap���б任�ϲ�Ϊһ��SplatMap
		*/
		static void ConvertSplatMap(const std::string& dirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& targetFilename);
	
		/*
		* ������ConvertSplatMap����������SplatMap���Ӷ���֤����ȷ��
		*/
		static void ResolveConvertedSplatMap(const std::string& filename, const std::string& targetFilename);

		/*
		* ��dirname�µ�����ͼ��ϲ�Ϊһ��TextureArray��������MipChain�����һ����MipChain�ϵ�ÿһ������Դ�Ĵ�С������ڵ���64kb
		*/
		static void GenerateTextureArray(const std::string& srcDirname, const std::string& dstFilename);

		/*
		* ����һ�������ɫͼ
		*/
		static void GenerateRandomAlbedoMap(const std::string& dstFilename, uint32_t width, uint32_t height);

		/*
		* ����ÿһ�����νڵ������
		*/
		static void GenerateTerrainQuadNodeDescriptors(const std::string& heightMapFilename, const std::string& dstDirname);

		/*
		* ���ݵ��θ߶�ͼ���ɶ���������
		*/
		static void GenerateHeightBinData(const std::string& heightMapFilename, const std::string& binDataFilename);
	};

}
