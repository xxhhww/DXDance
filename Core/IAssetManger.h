#pragma once
#include <unordered_map>
#include <string>
#include <memory>

namespace Core {
	/*
	* 资产管理接口类，其子模板必须是IAsset的子类 
	* 该类及其子类并不从外部解析资产，资产的解析工作由AssetLoader来完成
	*/
	template<typename TAsset>
	class IAssetManger {
	public:
		/*
		* 构造函数，设置资产路径
		*/
		IAssetManger(const std::string& assetPath, const std::string& enginePath);

		/*
		* 通过id判断资产是否存在
		*/
		bool IsRegistered(int64_t id);

		/*
		* 通过name判断资产是否存在
		*/
		bool IsRegistered(const std::string& name);
		
		/*
		* 通过id获取资产指针
		*/
		TAsset* GetResource(int64_t id);

		/*
		* 通过name获取资产指针
		*/
		TAsset* GetResource(const std::string& name);

		/*
		* 通过用户的操作来注册资源
		*/
		virtual void RegisterResource(TAsset* target) = 0;

		/*
		* 通过指定的路径(必须是项目路径或者引擎路径)来解析并管理资源.
		* 其他路径的资源由AssetLoader解析，并通过RegisterResource()方法来注册进管理类.
		*/
		virtual TAsset* LoadResource(const std::string& path) = 0;
	protected:
		std::unordered_map<int64_t, std::unique_ptr<TAsset>> mAssets;
		std::string mAssetPath{ "" };
		std::string mEnginePath{ "" };
	};
}

#include "IAssetManger.inl"