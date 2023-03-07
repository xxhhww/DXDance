#pragma once
#include "Scene.h"


namespace Core {
	/*
	* 场景管理器
	* 场景管理器不会在一个开始就将所有场景文件读取到内存中，而是只从硬盘中读取当前使用的场景
	* 对于不使用的场景，管理器会将它从内存中析构，该场景使用的Asset也会引用计数减一，
	*/
	class SceneManger {
	public:
		/*
		* 构造函数
		*/
		SceneManger() = default;

		/*
		* 析构函数
		*/
		~SceneManger();

		/*
		* 卸载当前使用的场景，卸载时将自动保存至磁盘中
		*/
		void UnLoadCurrentScene();

		/*
		* 创建一个空场景
		*/
		void CreateEmptyScene();

		/*
		* 从磁盘中读取目标场景
		* @Param abPath: 场景文件的绝对路径
		*/
		void LoadSceneFromDisk(const std::string& abPath);

	private:
		Scene* mCurrScene{ nullptr };	// 当前正在使用的场景
	};
}