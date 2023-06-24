#include "Adapter.h"
#include "Tools/Assert.h"
#include "AdapterContainer.h"

namespace GHL {
    Adapter::Adapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter) 
    : mAdapter(adapter){
        RefetchDisplaysIfNeeded();
    }

    void Adapter::RefetchDisplaysIfNeeded() {
        DXGI_ADAPTER_DESC1 desc{};
        mAdapter->GetDesc1(&desc);

        bool refetchNeeded = (!mDXGIFactory || !mDXGIFactory->IsCurrent());

        if (!refetchNeeded) {
            return;
        }

        HRASSERT(CreateDXGIFactory2(0, IID_PPV_ARGS(&mDXGIFactory)));

        mConnectedDisplays.clear();

        uint32_t i = 0;
        Microsoft::WRL::ComPtr<IDXGIOutput> currentOutput;

        while (mAdapter->EnumOutputs(i, &currentOutput) != DXGI_ERROR_NOT_FOUND) {
            Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
            HRASSERT(currentOutput.As(&output6));

            DXGI_OUTPUT_DESC1 desc;
            HRASSERT(output6->GetDesc1(&desc));

            mConnectedDisplays.emplace_back(desc);
            i++;
        }
    }
}