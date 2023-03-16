#pragma once
#include "Adapter.h"
#include <memory>

namespace GHL {
	/*
	* ������������һ̨�������ж��������
	*/
	class AdapterContainer {
	public:
		/*
		* Ĭ�Ϲ��캯��
		*/
		AdapterContainer();

		inline const auto  DXGIFactory()    const { return mDXGIFactory.Get(); }

		/*
		* ��ȡ���������
		*/
		inline const auto& GetWARPAdapter() const { return *mWARPAdapter.get(); }

		/*
		* ��ȡӲ��������
		*/
		const Adapter& GetHardwareAdapter(int32_t idx) const;

		/*
		* ��ȡ������Ӳ��������
		*/
		const Adapter& GetHighPerformanceAdapter() const;

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory2> mDXGIFactory;
		std::unique_ptr<Adapter> mWARPAdapter;
		std::vector<Adapter> mHardwareAdapters;
	};
}