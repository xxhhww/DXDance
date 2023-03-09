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

        /*
        * Get����
        */
        inline const auto  DXGIFactory() const { return mDXGIFactory.Get(); }
        inline const auto  DXGIAdapter() const { return mAdapter.Get(); }
        inline const auto& GetDisplays() const { return mConnectedDisplays; }
    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;
        std::vector<Display> mConnectedDisplays;
    };
}