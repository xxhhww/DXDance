#pragma once
#include "AssetPathDataBase.h"

namespace Core {
	/*
	* �ʲ�����ӿ���
	*/
	template<typename TAsset>
	class IAssetManger {
	public:
		/*
		* ���캯���������ʲ�·��
		*/
		IAssetManger(AssetPathDataBase* dataBase, const std::string& assetPath, const std::string& enginePath, bool enableUnload);

		/*
		* ��������
		*/
		virtual ~IAssetManger();

		/*
		* ����enableUnload
		*/
		void SetEnableUnload(bool enableUnload);

		/*
		* ͨ��path�ж��ʲ��Ƿ����
		* @Param path: ���·��
		*/
		bool IsRegistered(const std::string& path);
		
		/*
		* �ʲ����ü�����һ������Դ�����ڻ�δ����ʱ��������Դ
		*/
		TAsset* UseResource(int64_t id);

		/*
		* �ʲ����ü�����һ������Դ�����ڻ�δ����ʱ��������Դ
		* @Param path: ���·��
		*/
		TAsset* UseResource(const std::string& path);

		/*
		* �ʲ����ü�����һ������Դ���ü���Ϊ0����֧����Դж��ʱ��ж����Դ
		*/
		void UnuseResource(TAsset* asset);

		/*
		* ������Դ�����·��
		* @Param oldPath: �ɵ����·��
		* @Param newPath: �µ����·��
		*/
		void RepathResource(const std::string& oldPath, const std::string& newPath);

		/*
		* �����Դ�ľ���·��
		*/
		std::string GetRealPath(const std::string& path);

	protected:
		AssetPathDataBase*	mPathDataBase{ nullptr };		// �ʲ���·����
		std::string			mAssetPath;						// �ʲ�·��
		std::string			mEnginePath;					// ����·��
		bool				mEnableUnload{ true };			// �Ƿ�֧����Դж��
		std::unordered_map<std::string, TAsset*> mAssets;	// ���·��-�ʲ�Map
	};
}

#include "IAssetManger.inl"