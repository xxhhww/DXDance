#pragma once
#include <unordered_map>
#include <string>
#include <atomic>
#include "ISerializable.h"

namespace Core {
	// 资产接口类
	class IAsset : public IJsonSerializable {
	public:
		// Create From JsonFile
		inline IAsset() = default;
		// Create From Runtime
		inline IAsset(const std::string& name) : mName(name), mID(smAtomicIncID++) {}
		virtual ~IAsset() = default;

		inline const auto& GetName()	const { return mName; }
		inline const auto& GetID()		const { return mID; }
	protected:
		std::string mName{ "" };
		int64_t		mID{ -1 };
		static std::atomic<int64_t> smAtomicIncID;
	};

	// IAssetManger的子类模板必须是IAsset的子类
	template<typename TAsset>
	class IAssetManger {
	public:
		bool IsRegistered(int64_t id);
		bool IsRegistered(const std::string& name);
		
		TAsset* GetResource(int64_t id);
		TAsset* GetResource(const std::string& name);

		static void SetAssetPath(const std::string& path);
	private:
		std::unordered_map<int64_t, TAsset*> mAssets;
		inline static std::string smAssetPath = "";
	};
}

#include "IAssetManger.inl"