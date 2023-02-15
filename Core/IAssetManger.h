#pragma once
#include <unordered_map>
#include <string>
#include <atomic>
#include <memory>
#include "Tools/ISerializable.h"

namespace Core {
	/* 资产接口类 */
	class IAsset : public Tool::ISerializable {
	public:
		/*
		* 默认构造函数，用于资产从文件中读取的情景
		*/
		IAsset() = default;

		/*
		* 构造函数，用于资产在编辑器运行时创建的情景，需要提供资产名称
		*/
		inline IAsset(const std::string& name) : mName(name), mID(smAtomicIncID++) {}

		/*
		* 默认虚析构
		*/
		virtual ~IAsset() = default;

		inline const auto& GetName()	const { return mName; }
		inline const auto& GetID()		const { return mID; }

	protected:
		std::string mName{ "?" };
		int64_t		mID{ -1 };
		inline static std::atomic<int64_t> smAtomicIncID = 0;
	};

	/* 资产管理接口类，其子模板必须是IAsset的子类 */
	template<typename TAsset>
	class IAssetManger {
	public:
		/*
		* 构造函数，设置资产路径
		*/
		IAssetManger(const std::string& path);
		
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

	protected:
		std::unordered_map<int64_t, std::unique_ptr<TAsset>> mAssets;
		std::string mAssetPath{ "" };
	};
}

#include "IAssetManger.inl"