#include "AdapterContainer.h"
#include "Tools/Assert.h"
#include <algorithm>

namespace GHL {
	AdapterContainer::AdapterContainer() {
        HRASSERT(CreateDXGIFactory(IID_PPV_ARGS(&mDXGIFactory)));

        uint32_t i = 0;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        while (mDXGIFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC1 desc;
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

	const Adapter& AdapterContainer::GetHardwareAdapter(int32_t idx) const {
        idx = std::clamp(idx, 0, (int32_t)mHardwareAdapters.size());
        return mHardwareAdapters.at(idx);
	}

    const Adapter& AdapterContainer::GetHighPerformanceAdapter() const {
        return mHardwareAdapters.at(0);
    }
}