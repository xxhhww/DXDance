#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"
#include <queue>

namespace GHL {
	/*
	* ���������
	*/
	class DescriptorHandle {
	public:
		DescriptorHandle() = default;
		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, size_t heapIndex);
		~DescriptorHandle() = default;

		/*
		* Get����
		*/
		const auto& GetHeapIndex() const { return mHeapIndex & 0x0000FFFF; }
		const auto& GetHeapType()  const { return D3D12_DESCRIPTOR_HEAP_TYPE(mHeapIndex >> 32); }
		const auto& GetCpuHandle() const { return mCpuHandle; }
		const auto& GetGpuHandle() const { return mGpuHandle; }

		/*
		* ����������
		*/
		operator D3D12_CPU_DESCRIPTOR_HANDLE const() const { return mCpuHandle; }
		operator D3D12_GPU_DESCRIPTOR_HANDLE const() const { return mGpuHandle; }

	private:
		size_t mHeapIndex{ 0u };	// �������ڶ��ϵ�����
		D3D12_CPU_DESCRIPTOR_HANDLE mCpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
		D3D12_GPU_DESCRIPTOR_HANDLE mGpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
	};

	/*
	* ��������
	*/
	class DescriptorHeap : public D3DObject {
	public:
		DescriptorHeap(const Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t capacity);
		~DescriptorHeap() = default;

		/*
		* ����һ��������
		*/
		DescriptorHandle Allocate(uint64_t heapIndex);

		/*
		* Get����
		*/
		inline const auto& GetHeapType()       const { return mType; }
		inline const auto& GetIncrementSize()  const { return mIncrementSize; }
		inline const auto& GetHeapDesc()       const { return mDesc; }
		inline const auto& GetCapacity()       const { return mCapacity; }
		inline const auto  D3DDescriptorHeap() const { return mHeap.Get(); }

		/*
		* �����������ѵĵ�������
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* ��ȡD3DObject�ĵ�������
		*/
		const std::string& GetDebugName() override;

	private:
		const Device* mDevice{ nullptr };
		std::string mName;
		D3D12_DESCRIPTOR_HEAP_TYPE mType;
		uint32_t mIncrementSize{ 0u };
		D3D12_DESCRIPTOR_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;

		uint64_t mCapacity{ 0u };	// ����
		D3D12_CPU_DESCRIPTOR_HANDLE mStartCpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
		D3D12_GPU_DESCRIPTOR_HANDLE mStartGpuHandle{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
	};
}