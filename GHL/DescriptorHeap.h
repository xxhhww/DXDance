#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"
#include <queue>

namespace GHL {
	/*
	* 描述符句柄
	*/
	class DescriptorHandle {
	public:
		DescriptorHandle() = default;
		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, size_t heapIndex);
		~DescriptorHandle() = default;

		/*
		* Get方法
		*/
		const auto& GetHeapIndex() const { return mHeapIndex & 0x0000FFFF; }
		const auto& GetHeapType()  const { return mHeapIndex >> 32; }
		const auto& GetCpuHandle() const { return mCpuHandle; }
		const auto& GetGpuHandle() const { return mGpuHandle; }

		/*
		* 操作符重载
		*/
		operator D3D12_CPU_DESCRIPTOR_HANDLE const() const { return mCpuHandle; }
		operator D3D12_GPU_DESCRIPTOR_HANDLE const() const { return mGpuHandle; }

	private:
		size_t mHeapIndex{ 0u };	// 描述符在堆上的索引
		D3D12_CPU_DESCRIPTOR_HANDLE mCpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
		D3D12_GPU_DESCRIPTOR_HANDLE mGpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
	};

	/*
	* 描述符堆
	*/
	class DescriptorHeap : public D3DObject {
	public:
		DescriptorHeap(const Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t capacity);
		~DescriptorHeap() = default;

		/*
		* 分配一个描述符
		*/
		DescriptorHandle Allocate();

		/*
		* 回收一个描述符
		*/
		void Deallocate(uint64_t heapIndex);

		/*
		* Get方法
		*/
		inline const auto& GetHeapType()       const { return mType; }
		inline const auto& GetIncrementSize()  const { return mIncrementSize; }
		inline const auto& GetHeapDesc()       const { return mDesc; }
		inline const auto& GetCapacity()       const { return mCapacity; }
		inline const auto  D3DDescriptorHeap() const { return mHeap.Get(); }

		/*
		* 设置描述符堆的调试名称
		*/
		void SetDebugName(const std::string& name) override;

	private:
		const Device* mDevice{ nullptr };
		D3D12_DESCRIPTOR_HEAP_TYPE mType;
		uint32_t mIncrementSize{ 0u };
		D3D12_DESCRIPTOR_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;

		uint64_t mCapacity{ 0u };	// 容量
		uint64_t mCurrIndex{ 0u };	// 当前使用的索引
		std::priority_queue<uint64_t> mRetiredIndices;	// 不使用的索引
		D3D12_CPU_DESCRIPTOR_HANDLE mStartCpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
		D3D12_GPU_DESCRIPTOR_HANDLE mStartGpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
	};
}