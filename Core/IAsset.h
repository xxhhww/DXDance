#pragma once
#include "Tools/JsonHelper.h"
#include <atomic>
#include <string>

namespace Core {
	class IAssetManger;

	/*
	* 资产状态
	*/
	enum class AssetStatus {
		UnLoad,	// 未加载
		Loaded	// 已加载
	};

	/* 资产接口类 */
	class IAsset {
	public:
		/*
		* 默认构造函数
		*/
		IAsset(IAssetManger* assetManger);

		/*
		* 默认虚析构
		*/
		virtual ~IAsset();

		/*
		* 加载
		*/
		virtual void Load(const std::string& path, bool aSync = false) = 0;

		/*
		* 卸载
		*/
		virtual void UnLoad(const std::string& path) = 0;

		/*
		* 引用计数加一
		*/
		inline uint32_t IncRefCount() { return ++mRefCount; }

		/*
		* 引用计数减一
		*/
		inline uint32_t DecRefCount() { return --mRefCount; }

		/*
		* Set方法
		*/
		inline void SetName	(const std::string& name)	{ mName = name; }
		inline void SetUID	(int64_t id)				{ mID = id; }

		/*
		* Get方法
		*/
		inline const auto& GetName()	const { return mName; }
		inline const auto& GetUID()		const { return mID; }
		inline const auto& GetStatus()	const { return mStatus; }
	protected:
		IAssetManger*	mManger{ nullptr };		// 资产管理器
		std::string		mName{ "" };			// 资产名称
		int64_t			mID{ -1 };				// 资产ID
		AssetStatus		mStatus;				// 资产状态
		std::atomic<uint32_t> mRefCount{ 1u };	// 引用计数
	};
}