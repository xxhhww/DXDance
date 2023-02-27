#pragma once
#include "IAssetManger.h"

namespace Core {
	template<typename TAsset>
	IAssetManger<TAsset>::IAssetManger(const std::string& path, const std::string& enginePath)
	: mAssetPath(path) 
	, mEnginePath(enginePath) {}
	
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
			[&id](std::pair<const int64_t, std::unique_ptr<TAsset>>& pair) {
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
			[&name](std::pair<const int64_t, std::unique_ptr<TAsset>>& pair) {
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

	template<typename TAsset>
	void IAssetManger<TAsset>::UnRegisterResource(int64_t id) {
		if (!IsRegistered(id)) {
			return;
		}
		auto it = mAssets.at(id);
		mAssets.erase(it);
	}

	template<typename TAsset>
	void IAssetManger<TAsset>::UnRegisterResource(const std::string& name) {
		auto it = std::find_if(mAssets.begin(), mAssets.end(),
			[&name](std::pair<const int64_t, std::unique_ptr<TAsset>>& pair) {
				if (pair.second->GetName() == name) {
					return true;
				}
				return false;
			});

		if (it != mAssets.end()) {
			mAssets.erase(it);
		}
	}

	template<typename TAsset>
	void IAssetManger<TAsset>::RenameResource(const std::string& oldName, const std::string& newName) {
		for (auto& pair : mAssets) {
			if (pair.second->GetName() == oldName) {
				pair.second->SetName(newName);
			}
		}
	}
}