#pragma once
#include "AssetPathDataBase.h"

namespace Core {
	class IAsset;

	/*
	* �ʲ�����ӿ���
	*/
	class IAssetManger {
	public:
		/*
		* ���캯���������ʲ�·��
		*/
		IAssetManger(const std::string& assetPath, const std::string& enginePath);

		/*
		* ��������
		*/
		virtual ~IAssetManger();

		/*
		* ͨ��name�ж��ʲ��Ƿ����
		*/
		bool IsRegistered(const std::string& name);
		
		/*
		* �ʲ����ü�����һ
		*/
		IAsset* UseResource(int64_t id);

		/*
		* �ʲ����ü�����һ
		*/
		IAsset* UseResource(const std::string& path);

		/*
		* �ʲ����ü�����һ
		*/
		void UnUseResource(IAsset* asset);

		/*
		* ������Դ����
		*/
		void RenameResource(const std::string& oldName, const std::string& newName);

		/*
		* ������Դ·��
		*/
		void RepathResource(const std::string& oldPath, const std::string& newPath);

	protected:
		std::unordered_map<std::string, IAsset*> mAssets;	// ����-�ʲ�Map
		std::string mAssetPath;		// �ʲ��ļ�·��
		std::string mEnginePath;	// �����ļ�·��
	};
}