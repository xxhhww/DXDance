#include "AdapterContainer.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"
#include <algorithm>

namespace GHL {
	AdapterContainer::AdapterContainer() {
        HRASSERT(CreateDXGIFactory(IID_PPV_ARGS(&mDXGIFactory)));

        uint32_t i = 0;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        while (mDXGIFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                mWARPAdapter = std::make_unique<Adapter>(adapter);
            }
            else {
                mHardwareAdapters.emplace_back(adapter);
            }

            ++i;
        }
	}

	const Adapter* AdapterContainer::GetHardwareAdapter(int32_t idx) const {
        idx = std::clamp(idx, 0, (int32_t)mHardwareAdapters.size());
        return &mHardwareAdapters.at(idx);
	}

    const Adapter* AdapterContainer::GetHighPerformanceAdapter() const {
        for (uint32_t i = 0; i < mHardwareAdapters.size(); i++) {
            DXGI_ADAPTER_DESC1 desc{};
            mHardwareAdapters.at(i).DXGIAdapter()->GetDesc1(&desc);
            if (IsHighPerformanceAdapter(std::wstring(desc.Description))) {
                return &mHardwareAdapters.at(i);
            }
        }
        ASSERT_FORMAT(false, "High Performance Adapter Not Found");
        return nullptr;
    }

    bool AdapterContainer::IsHighPerformanceAdapter(const std::wstring& adapterName) {
        if (adapterName.find(L"NVIDIA GeForce RTX 3060 Laptop GPU") != std::wstring::npos) {
            return true;
        }
        return false;
    }
}