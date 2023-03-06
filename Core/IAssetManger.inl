#pragma once
#include "IAssetManger.h"
#include "AssetPathDataBase.h"
#include "ServiceLocator.h"
#include "Tools/StrUtil.h"

namespace Core {
	template<typename TAsset>
	IAssetManger<TAsset>::IAssetManger(const std::string& path, const std::string& enginePath) {}

	template<typename TAsset>
	IAssetManger<TAsset>::~IAssetManger() {

	}

	template<typename TAsset>
	bool IAssetManger<TAsset>::IsRegistered(const std::string& name) {
		return mAssets.find(name) != mAssets.end();
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::UseResource(int64_t id) {
		std::string path = CORESERVICE(AssetPathDataBase).GetPath(id);

		if (path.empty()) {
			return nullptr;
		}

		return UseResource(path);
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::UseResource(const std::string& path) {
		// 获得资源名称
		std::string assetname = Tool::StrUtil::RemoveBasePath(path);

		// 资产未注册
		if (!IsRegistered(assetname)) {
			mAssets[assetname] = new TAsset(this);
		}

		if (IsRegistered(assetname)) {
			IAsset* asset = mAssets.at(assetname);

			// 资源已被加载
			if (asset->GetStatus() == AssetStatus::Loaded) {
				return asset;
			}

			// 资源存在，但是未加载
			asset.
		}
	}

	template<typename TAsset>
	void IAssetManger<TAsset>::UnUseResource(TAsset* asset) {

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