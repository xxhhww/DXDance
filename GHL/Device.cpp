#include "Device.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
	Device::Device(const Adapter& adapter, bool aftermathEnabled) 
	: mAftermathEnabled(aftermathEnabled)
	, mMinimumHeapSize(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) 
	, mHeapAlignment(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_1;
        HRASSERT(D3D12CreateDevice(adapter.DXGIAdapter(), featureLevel, IID_PPV_ARGS(&mDevice)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS featureSupport{};
        HRASSERT(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureSupport, sizeof(featureSupport)));

        switch (featureSupport.ResourceHeapTier) {
        case D3D12_RESOURCE_HEAP_TIER_1: mSupportUniversalHeaps = false; break;
        default: mSupportUniversalHeaps = true; break;
        }

        // EnableAftermath(mAftermathEnabled);
	}

	void Device::EnableAftermath(bool enable) {
        HRASSERT(mDevice->SetStablePowerState(enable));
	}

    void Device::SetDebugName(const std::string& name) {
        HRASSERT(mDevice->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
    }
}