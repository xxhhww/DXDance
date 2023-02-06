#pragma once
#include "IAssetManger.h"

namespace Core {
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
		if (IsRegistered(id)) {
			return mAssets[id];
		}
		return nullptr;
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::GetResource(const std::string& name) {
		for (auto& pair : mAssets) {
			if (pair.second->GetName() == name) {
				return pair.second;
			}
		}
		return nullptr;
	}

	template<typename TAsset>
	void IAssetManger<TAsset>::SetAssetPath(const std::string& path) {
		smAssetPath = path;
	}
}