#pragma once
#include "Scene.h"
#include "AssetPathDataBase.h"

namespace Core {

	/*
	* 场景管理器
	* 场景管理器不会在一个开始就将所有场景文件读取到内存中，而是只从硬盘中读取当前使用的场景
	* 对于不使用的场景，管理器会将它从内存中析构，该场景使用的Asset也会引用计数减一，
	*/
	class SceneManger {
	public:
		SceneManger(const std::string& assetPath, const std::string& enginePath, AssetPathDataBase* pathDataBase);
		~SceneManger();

		/*
		* 卸载当前使用的场景，卸载时将自动保存至磁盘中
		*/
		void UnLoadCurrentScene();

		/*
		* 保存当前场景
		*/
		void SaveCurrentScene();

		/*
		* 创建一个空场景
		*/
		void CreateEmptyScene(const std::string& path);

		/*
		* 从磁盘中读取目标场景
		* @Param path: 场景文件的相对路径(以Engine或者Asset开头)
		*/
		void LoadSceneFromDisk(const std::string& path);

		/*
		* 获得当前场景
		*/
		inline auto* GetCurrentScene() const { return mCurrScene; }

		/*
		* 获取资产的完整路径
		*/
		std::string GetFullPath(const std::string& path);
		std::string GetFullPath(int64_t uid);
	private:
		const std::string mAssetPath;
		const std::string mEnginePath;
		AssetPathDataBase* mAssetPathDataBase{ nullptr };

		Scene* mCurrScene{ nullptr };	// 当前正在使用的场景
	};

}