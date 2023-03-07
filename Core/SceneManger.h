#pragma once
#include "Scene.h"


namespace Core {
	/*
	* ����������
	* ����������������һ����ʼ�ͽ����г����ļ���ȡ���ڴ��У�����ֻ��Ӳ���ж�ȡ��ǰʹ�õĳ���
	* ���ڲ�ʹ�õĳ������������Ὣ�����ڴ����������ó���ʹ�õ�AssetҲ�����ü�����һ��
	*/
	class SceneManger {
	public:
		/*
		* ���캯��
		*/
		SceneManger() = default;

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
		* @Param abPath: �����ļ��ľ���·��
		*/
		void LoadSceneFromDisk(const std::string& abPath);

	private:
		Scene* mCurrScene{ nullptr };	// ��ǰ����ʹ�õĳ���
	};
}