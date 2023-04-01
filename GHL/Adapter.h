#pragma once
#include "pbh.h"
#include "Display.h"
#include <vector>

namespace GHL {
    /*
    * 适配器
    */
    class Adapter {
    public:
        /*
        * 构造函数
        */
        Adapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter);

        /*
        * 重新获取显示器
        */
        void RefetchDisplaysIfNeeded();


        inline const auto  DXGIFactory() const { return mDXGIFactory.Get(); }
        inline const auto  DXGIAdapter() const { return mAdapter.Get(); }
        inline const auto& GetDisplays() const { return mConnectedDisplays; }

        inline const auto& GetDisplay()  const { return mConnectedDisplays.at(0); }

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;
        std::vector<Display> mConnectedDisplays;
    };
}