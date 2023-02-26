#pragma once
#include "Tools/ISerializable.h"
#include <atomic>

namespace Core {
	/* 资产接口类 */
	class IAsset : public Tool::ISerializable {
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
		* Set方法
		*/
		inline void SetName	(const std::string& name)	{ mName = name; }
		inline void SetUID	(int64_t id)				{ mID = id; }

		/*
		* Get方法
		*/
		inline const auto& GetName()	const { return mName; }
		inline const auto& GetUID()		const { return mID; }

	protected:
		std::string mName{ "" };
		int64_t		mID{ -1 };
	};
}