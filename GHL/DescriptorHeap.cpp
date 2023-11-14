#include "DescriptorHeap.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

namespace GHL {
	DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, size_t heapIndex)
	: mCpuHandle(cpuHandle)
	, mGpuHandle(gpuHandle)
	, mHeapIndex(heapIndex) {}

	DescriptorHeap::DescriptorHeap(const Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t capacity)
	: mDevice(device)
	, mType(type)
	, mIncrementSize(mDevice->D3DDevice()->GetDescriptorHandleIncrementSize(mType))
	, mCapacity(capacity) {
		mDesc.NumDescriptors = mCapacity;
		mDesc.Type = mType;
		mDesc.NodeMask = mDevice->GetNodeMask();
		bool isShaderVisible =
			mType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
			mType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		mDesc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRASSERT(device->D3DDevice()->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&mHeap)));

		mStartCpuHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
		if (mType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || mType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
			mStartGpuHandle = mHeap->GetGPUDescriptorHandleForHeapStart();
		}
	}

	DescriptorHandle DescriptorHeap::Allocate(uint64_t heapIndex) {
		if (mType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || mType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {
			return DescriptorHandle{
				D3D12_CPU_DESCRIPTOR_HANDLE{ mStartCpuHandle.ptr + mIncrementSize * heapIndex },
				D3D12_GPU_DESCRIPTOR_HANDLE{ D3D12_GPU_VIRTUAL_ADDRESS_NULL },
				heapIndex | ( (uint64_t)mType << 32 )
			};
		}

		return DescriptorHandle{
			D3D12_CPU_DESCRIPTOR_HANDLE{ mStartCpuHandle.ptr + mIncrementSize * heapIndex },
			D3D12_GPU_DESCRIPTOR_HANDLE{ mStartGpuHandle.ptr + mIncrementSize * heapIndex },
			heapIndex | ( (uint64_t)mType << 32 )
		};
	}

	void DescriptorHeap::SetDebugName(const std::string& name) {
		mName = name;
		mHeap->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
	}

	const std::string& DescriptorHeap::GetDebugName() {
		return mName;
	}

}