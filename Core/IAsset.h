#pragma once
#include "Tools/JsonHelper.h"
#include <atomic>
#include <string>

namespace Core {
	/*
	* 资产状态
	*/
	enum class AssetStatus {
		UnLoad,	// 未加载
		Loaded	// 已加载
	};

	/* 资产接口类，资产的内存管理由资产管理类来完成 */
	class IAsset {
	public:
		/*
		* 默认构造函数
		*/
		IAsset() = default;

		/*
		* 默认虚析构
		*/
		virtual ~IAsset() = default;

		/*
		* 加载
		*/
		virtual void Load(bool aSync = false) = 0;

		/*
		* 卸载
		*/
		virtual void Unload() = 0;

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
		inline void SetPath	(const std::string& path)	{ mPath = path; }
		inline void SetUID	(int64_t id)				{ mID = id; }

		/*
		* Get方法
		*/
		inline const auto& GetPath()	const { return mPath; }
		inline const auto& GetUID()		const { return mID; }
		inline const auto& GetStatus()	const { return mStatus; }

	protected:
		std::string		mPath;					// 资产路径
		int64_t			mID{ -1 };				// 资产ID
		std::atomic<uint32_t> mRefCount{ 1u };	// 引用计数
		AssetStatus		mStatus{ AssetStatus::UnLoad };	// 资产状态
	};
}