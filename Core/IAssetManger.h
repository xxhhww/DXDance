#pragma once
#include "AssetPathDataBase.h"

namespace Core {
	class IAsset;

	/*
	* 资产管理接口类
	*/
	class IAssetManger {
	public:
		/*
		* 构造函数，设置资产路径
		*/
		IAssetManger(const std::string& assetPath, const std::string& enginePath);

		/*
		* 析构函数
		*/
		virtual ~IAssetManger();

		/*
		* 通过name判断资产是否存在
		*/
		bool IsRegistered(const std::string& name);
		
		/*
		* 资产引用计数加一
		*/
		IAsset* UseResource(int64_t id);

		/*
		* 资产引用计数加一
		*/
		IAsset* UseResource(const std::string& path);

		/*
		* 资产引用计数减一
		*/
		void UnUseResource(IAsset* asset);

		/*
		* 更改资源名称
		*/
		void RenameResource(const std::string& oldName, const std::string& newName);

		/*
		* 更改资源路径
		*/
		void RepathResource(const std::string& oldPath, const std::string& newPath);

	protected:
		std::unordered_map<std::string, IAsset*> mAssets;	// 名称-资产Map
		std::string mAssetPath;		// 资产文件路径
		std::string mEnginePath;	// 引擎文件路径
	};
}