#include "DescriptorAllocator.h"
#include "Tools/Assert.h"

namespace GHL {
	DescriptorAllocator::DescriptorAllocator(const Device* device, std::vector<uint64_t> capacity)
	: mDevice(device) 
	, mCBVSRVUAVDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, capacity.at(0))
	, mSamplerDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, capacity.at(1))
	, mRTVDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, capacity.at(2))
	, mDSVDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, capacity.at(3)) {}

	DescriptorHandle DescriptorAllocator::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type) {

		switch (type) {
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return mCBVSRVUAVDescriptorHeap.Allocate();
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return mSamplerDescriptorHeap.Allocate();
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return mRTVDescriptorHeap.Allocate();
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return mDSVDescriptorHeap.Allocate();
		default:
			ASSERT_FORMAT(false, "Unsupported Descriptor Heap Type");
			return DescriptorHandle{
				D3D12_CPU_DESCRIPTOR_HANDLE{ D3D12_GPU_VIRTUAL_ADDRESS_NULL },
				D3D12_GPU_DESCRIPTOR_HANDLE{ D3D12_GPU_VIRTUAL_ADDRESS_NULL },
				-1 };
		}
	}

	void DescriptorAllocator::Deallocate(DescriptorHandle& handle) {
		D3D12_DESCRIPTOR_HEAP_TYPE type = (D3D12_DESCRIPTOR_HEAP_TYPE)handle.GetHeapType();

		switch (type) {
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			mCBVSRVUAVDescriptorHeap.Deallocate(handle.GetHeapIndex());
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			mSamplerDescriptorHeap.Deallocate(handle.GetHeapIndex());
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			mRTVDescriptorHeap.Deallocate(handle.GetHeapIndex());
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			mDSVDescriptorHeap.Deallocate(handle.GetHeapIndex());
			break;
		default:
			ASSERT_FORMAT(false, "Unsupported Descriptor Heap Type");
			break;
		}
	}

}