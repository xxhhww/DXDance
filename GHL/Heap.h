#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"

namespace GHL {

	/*
	* 资源堆
	*/
	class Heap : public D3DObject {
	public:
		Heap(const Device* device, EResourceUsage usage);
		~Heap() = default;

		/*
		* Get方法
		*/
		inline const auto& GetHeapType() const { return mType; }
		inline const auto& GetHeapDesc() const { return mDesc; }
		inline const auto  D3DHeap()     const { return mHeap.Get(); }
	
		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

	private:
		const Device* mDevice{ nullptr };
		D3D12_HEAP_TYPE mType;
		D3D12_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12Heap> mHeap;
	};

}