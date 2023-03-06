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
		// �����Դ����
		std::string assetname = Tool::StrUtil::RemoveBasePath(path);

		// �ʲ�δע��
		if (!IsRegistered(assetname)) {
			mAssets[assetname] = new 
		}

		if (IsRegistered(assetname)) {
			IAsset* asset = mAssets.at(assetname);
			
			// ��Դ�ѱ�����
			if (asset->GetStatus() == AssetStatus::Loaded) {
				return asset;
			}

			// ��Դ���ڣ�����δ����
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