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
		Heap(const Device* device, size_t size, EResourceUsage usage);
		Heap(const Heap& other) = delete;
		Heap(Heap&& other) = default;
		~Heap() = default;

		Heap& operator=(const Heap& other) = delete;
		Heap& operator=(Heap&& other) = default;

		/*
		* Get方法
		*/
		inline const auto& GetUsage()    const { return mUsage; }
		inline const auto& GetHeapDesc() const { return mDesc; }
		inline const auto  D3DHeap()     const { return mHeap.Get(); }
	
		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* 获取D3DObject的调试名称
		*/
		const std::string& GetDebugName() override;

	private:
		const Device* mDevice{ nullptr };
		std::string mName;
		size_t mAlighnedSize;
		EResourceUsage mUsage;
		D3D12_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12Heap> mHeap;
	};

}