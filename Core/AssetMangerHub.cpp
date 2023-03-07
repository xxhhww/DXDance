#include "AssetMangerHub.h"

namespace Core {
	AssetMangerHub::AssetMangerHub(const std::string& assetPath, const std::string& enginePath)
	: mPathDataBase(assetPath + '\\' + "PathDataBase.json")
	, mAssetPath(assetPath)
	, mEnginePath(enginePath) {
		// ��ʼ�������ʲ�������
	}
}