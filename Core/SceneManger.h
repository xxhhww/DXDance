#pragma once
#include "Scene.h"
#include "AssetPathDataBase.h"

namespace Core {

	/*
	* ����������
	* ����������������һ����ʼ�ͽ����г����ļ���ȡ���ڴ��У�����ֻ��Ӳ���ж�ȡ��ǰʹ�õĳ���
	* ���ڲ�ʹ�õĳ������������Ὣ�����ڴ����������ó���ʹ�õ�AssetҲ�����ü�����һ��
	*/
	class SceneManger {
	public:
		SceneManger(const std::string& assetPath, const std::string& enginePath, AssetPathDataBase* pathDataBase);
		~SceneManger();

		/*
		* ж�ص�ǰʹ�õĳ�����ж��ʱ���Զ�������������
		*/
		void UnLoadCurrentScene();

		/*
		* ���浱ǰ����
		*/
		void SaveCurrentScene();

		/*
		* ����һ���ճ���
		*/
		void CreateEmptyScene(const std::string& path);

		/*
		* �Ӵ����ж�ȡĿ�곡��
		* @Param path: �����ļ������·��(��Engine����Asset��ͷ)
		*/
		void LoadSceneFromDisk(const std::string& path);

		/*
		* ��õ�ǰ����
		*/
		inline auto* GetCurrentScene() const { return mCurrScene; }

		/*
		* ��ȡ�ʲ�������·��
		*/
		std::string GetFullPath(const std::string& path);
		std::string GetFullPath(int64_t uid);
	private:
		const std::string mAssetPath;
		const std::string mEnginePath;
		AssetPathDataBase* mAssetPathDataBase{ nullptr };

		Scene* mCurrScene{ nullptr };	// ��ǰ����ʹ�õĳ���
	};

}