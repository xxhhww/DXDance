#include "IAssetManger.h"
#include "IAsset.h"
#include "Tools/StrUtil.h"

namespace Core {
	IAssetManger::IAssetManger(const std::string& assetPath, const std::string& enginePath) 
	: mAssetPath(assetPath) 
	, mEnginePath(enginePath) {}

	IAssetManger::~IAssetManger() {

	}

	bool IAssetManger::IsRegistered(const std::string& name) {
		return mAssets.find(name) != mAssets.end();
	}

	IAsset* IAssetManger::UseResource(int64_t id) {
		std::string path = smPathDataBase.GetPath(id);
		if (path.empty()) {
			return nullptr;
		}

		return UseResource(path);
	}

	IAsset* IAssetManger::UseResource(const std::string& path) {
		// 获得资源名称
		std::string assetname = Tool::StrUtil::RemoveBasePath(path);

		// 资产未注册
		if (!IsRegistered(assetname)) {
			mAssets[assetname] = new 
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

	void IAssetManger::UnUseResource(IAsset* asset) {

	}

	void IAssetManger::RenameResource(const std::string& oldName, const std::string& newName) {

	}

	void IAssetManger::RepathResource(const std::string& oldPath, const std::string& newPath) {

	}

	void IAssetManger::ProvidePathDataBase(const std::string& path) {

	}
}