#pragma once
#include "IAssetManger.h"
#include "Tools/StrUtil.h"

namespace Core {

	template<typename TAsset>
	IAssetManger<TAsset>::IAssetManger(AssetPathDataBase* dataBase, const std::string& assetPath, const std::string& enginePath, bool enableUnload)
	: mPathDataBase(dataBase)
	, mAssetPath(assetPath)
	, mEnginePath(enginePath)
	, mEnableUnload(enableUnload) {}

	template<typename TAsset>
	IAssetManger<TAsset>::~IAssetManger() {
		for (auto& pair : mAssets) {
			TAsset* asset = pair.second;
			asset->Unload();
			delete asset;
		}
		mAssets.clear();
	}

	template<typename TAsset>
	void IAssetManger<TAsset>::SetEnableUnload(bool enableUnload) {
		mEnableUnload = enableUnload;
	}

	template<typename TAsset>
	bool IAssetManger<TAsset>::IsRegistered(const std::string& path) {
		return mAssets.find(path) != mAssets.end();
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::UseResource(int64_t id) {
		std::string path = mPathDataBase.GetPath(id);
		if (path.empty()) {
			return nullptr;
		}

		return UseResource(path);
	}

	template<typename TAsset>
	TAsset* IAssetManger<TAsset>::UseResource(const std::string& path) {
		// ��Դδע��
		if (!IsRegistered(path)) {
			TAsset* newAsset = new TAsset(this);
			newAsset->SetPath(path);
			newAsset->SetUID(mPathDataBase->GetUID(path));

			mAssets[path] = newAsset;
		}

		TAsset* asset = mAssets.at(path);

		// ����
		asset->Load();

		// ������ü���
		asset->IncRefCount();

		return asset;
	}


	template<typename TAsset>
	void IAssetManger<TAsset>::UnuseResource(TAsset* asset) {
		uint32_t refCount = asset->DecRefCount();

		if (refCount == 0 && mEnableUnload) {
			asset->Unload();
		}
	}

	template<typename TAsset>
	void IAssetManger<TAsset>::RepathResource(const std::string& oldPath, const std::string& newPath) {
		if (oldPath == newPath) {
			return;
		}

		auto it = mAssets.find(oldPath);
		if (it == mAssets.end()) {
			return;
		}

		TAsset* asset = *it;
		asset->SetPath(newPath);

		mPathDataBase->SetPath(asset->GetUID(), newPath);
	}

	template<typename TAsset>
	std::string IAssetManger<TAsset>::GetRealPath(const std::string& path) {
		// ������·���� "Engine" ��ͷ�����������ļ�
		if (Tool::StrUtil::StartWith(path, "Engine")) {
			return mEnginePath + '\\' + path;
		}
		else if (Tool::StrUtil::StartWith(path, "Assets")) {
			return mAssetPath + '\\' + path;
		}
		
		return path;
	}

}