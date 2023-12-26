#pragma once
#include <string>

namespace OfflineTask {

	/*
	* �Ը߶�ͼ���е��úͷָ����
	*/
	class TextureProcessor {
	public:
		static void Padding(const std::string& filename, const std::string& dirname, uint32_t paddingSize);

		/*
		* ��ͼ����зָ�����ָ��Ľ�����뵽Ŀ��Ŀ¼��
		*/
		static void Split(const std::string& filename, const std::string& dirname, uint32_t subSize, uint32_t step, uint32_t startNameIndex);

		static void Resize(const std::string& filename, const std::string& dirname, uint32_t targetWidth, uint32_t targetHeight);

		/*
		* ��dirname�µ�����ͼ��ϲ�ΪTextureAtlasFile
		*/
		static void MergeTextureAtlasFile(const std::string& dirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& targetFilename);

	};

}
