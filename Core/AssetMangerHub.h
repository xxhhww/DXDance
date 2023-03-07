#pragma once
#include "AssetPathDataBase.h"
#include "Tools/StrUtil.h"

namespace Core {
	class AssetMangerHub {
	public:
		AssetMangerHub(const std::string& assetPath, const std::string& enginePath);
	private:
		AssetPathDataBase	mPathDataBase;	// UID-相对路径表
		std::string			mAssetPath;		// 用户资产路径
		std::string			mEnginePath;	// 引擎资产路径
		std::unordered_map<Tool::FileType, >
	};
}