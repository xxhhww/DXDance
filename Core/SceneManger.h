#pragma once
#include "IAssetManger.h"
#include "Scene.h"

namespace Core {
	/*
	* ����������
	* ����������������һ����ʼ�ͽ�ȫ��������ȡ���ڴ��У�����ֻ��Ӳ���ж�ȡ��ǰʹ�õĳ���
	* ���ڲ�ʹ�õĳ������������Ὣ�����ڴ����������ó���ʹ�õ�AssetҲ�����ü�����һ��
	*/
	class SceneManger : public IAssetManger<Scene> {
	public:

	private:
		Scene* mCurrScene{ nullptr };	// ��ǰ���ڴ���ĳ���
	};
}