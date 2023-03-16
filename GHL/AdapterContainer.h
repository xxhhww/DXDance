#pragma once
#include "Adapter.h"
#include <memory>

namespace GHL {
	/*
	* 适配器容器，一台主机会有多个适配器
	*/
	class AdapterContainer {
	public:
		/*
		* 默认构造函数
		*/
		AdapterContainer();

		inline const auto  DXGIFactory()    const { return mDXGIFactory.Get(); }

		/*
		* 获取软件适配器
		*/
		inline const auto& GetWARPAdapter() const { return *mWARPAdapter.get(); }

		/*
		* 获取硬件适配器
		*/
		const Adapter& GetHardwareAdapter(int32_t idx) const;

		/*
		* 获取高性能硬件适配器
		*/
		const Adapter& GetHighPerformanceAdapter() const;

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory2> mDXGIFactory;
		std::unique_ptr<Adapter> mWARPAdapter;
		std::vector<Adapter> mHardwareAdapters;
	};
}