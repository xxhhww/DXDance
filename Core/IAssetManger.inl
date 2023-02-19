#pragma once
#include "IAssetManger.h"

namespace Core {
	template<typename TAsset>
	IAssetManger<TAsset>::IAssetManger(const std::string& path) 
	: mAssetPath(path) {}

	/*
	* ×¢²á×ÊÔ´
	*/
	template<typename TAsset>
	void IAssetManger<TAsset>::RegisterResource(TAsset* target) {
		if (!GetResource(target->GetID()) && !GetResource(target->GetName())) {
			mAssets[target->GetID] = std::make_unique(target);
		}
	}
	
	template<typename TAsset>
	bool IAssetManger<TAsset>::IsRegistered(int64_t id) {
		return mAssets.find(id) != mAssets.end();
	}

	template<typename TAsset>
	bool IAssetManger<TAsset>::IsRegistered(const std::string& name) {
		for (const auto& pair : mAssets) {
			if (pair.second->GetName() == name) {
				return true;
			}
		}
		return false;
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::GetResource(int64_t id) {
		auto it = std::find_if(mAssets.begin(), mAssets.end(),
			[&id](const std::pair<int64_t, std::unique_ptr<TAsset>>& pair) {
				if (pair.first == id) {
					return true;
				}
				return false;
			});

		if (it != mAssets.end()) {
			return (*it).second.get();
		}
		return nullptr;
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::GetResource(const std::string& name) {
		auto it = std::find_if(mAssets.begin(), mAssets.end(),
			[&name](const std::pair<int64_t, std::unique_ptr<TAsset>>& pair) {
				if (pair.second->GetName() == name) {
					return true;
				}
				return false;
			});

		if (it != mAssets.end()) {
			return (*it).second.get();
		}
		return nullptr;
	}
}