#pragma once
#include "AssetPathDataBase.h"

namespace Core {
	/*
	* 资产管理接口类
	*/
	template<typename TAsset>
	class IAssetManger {
	public:
		/*
		* 构造函数，设置资产路径
		*/
		IAssetManger(AssetPathDataBase* dataBase, const std::string& assetPath, const std::string& enginePath, bool enableUnload);

		/*
		* 析构函数
		*/
		virtual ~IAssetManger();

		/*
		* 设置enableUnload
		*/
		void SetEnableUnload(bool enableUnload);

		/*
		* 通过path判断资产是否存在
		* @Param path: 相对路径
		*/
		bool IsRegistered(const std::string& path);
		
		/*
		* 资产引用计数加一，当资源不存在或未加载时，加载资源
		*/
		TAsset* UseResource(int64_t id);

		/*
		* 资产引用计数加一，当资源不存在或未加载时，加载资源
		* @Param path: 相对路径
		*/
		TAsset* UseResource(const std::string& path);

		/*
		* 资产引用计数减一，当资源引用计数为0并且支持资源卸载时，卸载资源
		*/
		void UnuseResource(TAsset* asset);

		/*
		* 更改资源的相对路径
		* @Param oldPath: 旧的相对路径
		* @Param newPath: 新的相对路径
		*/
		void RepathResource(const std::string& oldPath, const std::string& newPath);

		/*
		* 获得资源的绝对路径
		*/
		std::string GetRealPath(const std::string& path);

	protected:
		AssetPathDataBase*	mPathDataBase{ nullptr };		// 资产的路径表
		std::string			mAssetPath;						// 资产路径
		std::string			mEnginePath;					// 引擎路径
		bool				mEnableUnload{ true };			// 是否支持资源卸载
		std::unordered_map<std::string, TAsset*> mAssets;	// 相对路径-资产Map
	};
}

#include "IAssetManger.inl"