#pragma once
#include "Tools/ISerializable.h"
#include <atomic>

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

		inline void SetName(const std::string& name) { mName = name; }

		inline const auto& GetName()	const { return mName; }
		inline const auto& GetID()		const { return mID; }

	protected:
		std::string mName{ "?" };
		int64_t		mID{ -1 };
		inline static std::atomic<int64_t> smAtomicIncID = 0;
	};
}