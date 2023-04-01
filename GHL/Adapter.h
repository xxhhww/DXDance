#pragma once
#include "pbh.h"
#include "Display.h"
#include <vector>

namespace GHL {
    /*
    * ������
    */
    class Adapter {
    public:
        /*
        * ���캯��
        */
        Adapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter);

        /*
        * ���»�ȡ��ʾ��
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