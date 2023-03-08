#pragma once
#include "Scene.h"
#include "IAssetManger.h"


namespace Core {
	/*
	* ����������
	* ����������������һ����ʼ�ͽ����г����ļ���ȡ���ڴ��У�����ֻ��Ӳ���ж�ȡ��ǰʹ�õĳ���
	* ���ڲ�ʹ�õĳ������������Ὣ�����ڴ����������ó���ʹ�õ�AssetҲ�����ü�����һ��
	*/
	class SceneManger : public IAssetManger<Scene> {
	public:
		/*
		* ���캯��
		*/
		SceneManger(AssetPathDataBase* dataBase, const std::string& assetPath, const std::string& enginePath, bool enableUnload = true);

		/*
		* ��������
		*/
		~SceneManger();

		/*
		* ж�ص�ǰʹ�õĳ�����ж��ʱ���Զ�������������
		*/
		void UnLoadCurrentScene();

		/*
		* ����һ���ճ���
		*/
		void CreateEmptyScene();

		/*
		* �Ӵ����ж�ȡĿ�곡��
		* @Param path: �����ļ������·��
		*/
		void LoadSceneFromDisk(const std::string& path);

	private:
		Scene* mCurrScene{ nullptr };	// ��ǰ����ʹ�õĳ���
	};
}