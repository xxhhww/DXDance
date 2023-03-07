#pragma once
#include "AssetPathDataBase.h"
#include "Tools/StrUtil.h"

namespace Core {
	class AssetMangerHub {
	public:
		AssetMangerHub(const std::string& assetPath, const std::string& enginePath);
	private:
		AssetPathDataBase	mPathDataBase;	// UID-���·����
		std::string			mAssetPath;		// �û��ʲ�·��
		std::string			mEnginePath;	// �����ʲ�·��
		std::unordered_map<Tool::FileType, >
	};
}