#include "IAsset.h"

namespace Core {
	IAsset::IAsset(IAssetManger* assetManger) 
	: mManger(assetManger) 
	, mStatus(AssetStatus::UnLoad) {}

	IAsset::~IAsset() {

	}
}