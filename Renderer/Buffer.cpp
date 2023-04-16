#include "Buffer.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	Buffer::Buffer(
		const GHL::Device* device,
		const ResourceFormat& resFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator
		)
	: Resource(device, resFormat)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {

		const auto& bufferDesc = mResourceFormat.GetBufferDesc();

		D3D12_HEAP_PROPERTIES heapProperties{};
		if (bufferDesc.usage == GHL::EResourceUsage::Default) {
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		}
		else if (bufferDesc.usage == GHL::EResourceUsage::Upload) {
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		}
		else if (bufferDesc.usage == GHL::EResourceUsage::ReadBack) {
			heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
		}
		else {
			ASSERT_FORMAT(false, "Unsupport Resource Usage");
		}

		// HeapAllocator为空时，使用Committed方式创建资源
		if (mHeapAllocator == nullptr) {
			// 以默认方式创建该Buffer
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mResourceFormat.D3DResourceDesc(),
				GHL::GetD3DResourceStates(bufferDesc.initialState),
				nullptr,
				IID_PPV_ARGS(&mD3DResource)
			));
		}
		else {
			// 在堆上分配
			mHeapAllocation = mHeapAllocator->Allocate(bufferDesc.size);

			// 以放置方式创建该Buffer
			HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
				mHeapAllocation->heap->D3DHeap(),
				mHeapAllocation->heapOffset,
				&mResourceFormat.D3DResourceDesc(),
				GHL::GetD3DResourceStates(bufferDesc.initialState),
				nullptr,
				IID_PPV_ARGS(&mD3DResource)
			));
		}

		CreateDescriptor();

	}

	Buffer::Buffer(
		const GHL::Device* device,
		const ResourceFormat& resFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		const GHL::Heap* heap,
		size_t heapOffset)
	: Resource(device, resFormat)
	, mDescriptorAllocator(descriptorAllocator) {

		const auto& bufferDesc = mResourceFormat.GetBufferDesc();

		// 在堆上创建资源
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			heap->D3DHeap(),
			heapOffset,
			&mResourceFormat.D3DResourceDesc(),
			GHL::GetD3DResourceStates(bufferDesc.initialState),
			nullptr,
			IID_PPV_ARGS(&mD3DResource)
		));

		CreateDescriptor();

	}

	Buffer::~Buffer() {
		if (mHeapAllocation) {
			mHeapAllocator->Deallocate(mHeapAllocation);
			mHeapAllocation = nullptr;
		}
		UnMap();
	}

	uint8_t* Buffer::Map() {
		if (mMappedMemory) {
			return mMappedMemory;
		}

		const auto& bufferDesc = mResourceFormat.GetBufferDesc();

		D3D12_RANGE mapRange{ 0, bufferDesc.size };
		HRASSERT(mD3DResource->Map(0, &mapRange, (void**)&mMappedMemory));
		return mMappedMemory;
	}

	void Buffer::UnMap() {
		if (!mMappedMemory) {
			return;
		}

		mD3DResource->Unmap(0, nullptr);
		mMappedMemory = nullptr;
	}

	void Buffer::CreateDescriptor() {
		if (mDescriptorAllocator == nullptr) {
			return;
		}

		const auto& bufferDesc = mResourceFormat.GetBufferDesc();

		if (HasAllFlags(bufferDesc.expectedState, GHL::EResourceState::PixelShaderAccess) ||
			HasAllFlags(bufferDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess)) {
			// SRView
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			bool isAccelStruct = false;
			if (bufferDesc.format == DXGI_FORMAT_UNKNOWN) {
				if (HasAllFlags(bufferDesc.miscFlag, GHL::EBufferMiscFlag::RawBuffer)) {
					// This is a Raw Buffer
					srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					srvDesc.Buffer.FirstElement = 0u;
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					srvDesc.Buffer.NumElements = (UINT)bufferDesc.size / sizeof(uint32_t);
				}
				else if (HasAllFlags(bufferDesc.miscFlag, GHL::EBufferMiscFlag::StructuredBuffer)) {
					// This is a Structured Buffer
					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.Buffer.FirstElement = 0u;
					srvDesc.Buffer.NumElements = (UINT)bufferDesc.size / bufferDesc.stride;
					srvDesc.Buffer.StructureByteStride = bufferDesc.stride;
				}
				else if (HasAllFlags(bufferDesc.miscFlag, GHL::EBufferMiscFlag::AccelerateStruct)) {
					isAccelStruct = true;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
					srvDesc.RaytracingAccelerationStructure.Location = mD3DResource->GetGPUVirtualAddress();
				}
			}
			else {
				// This is a Typed Buffer
				uint32_t stride = GHL::GetFormatStride(bufferDesc.format);
				srvDesc.Format = bufferDesc.format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = 0u;
				srvDesc.Buffer.NumElements = (UINT)bufferDesc.size / stride;
			}

			mSRDescriptor = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDevice->D3DDevice()->CreateShaderResourceView(mD3DResource.Get(), &srvDesc, *mSRDescriptor.Get());
		}

		if (HasAllFlags(bufferDesc.expectedState, GHL::EResourceState::UnorderedAccess)) {
			// UAView
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;

			if (bufferDesc.format == DXGI_FORMAT_UNKNOWN) {
				if (HasAllFlags(bufferDesc.miscFlag, GHL::EBufferMiscFlag::RawBuffer)) {
					// This is a Raw Buffer
					uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)bufferDesc.size / sizeof(uint32_t);
				}
				else if (HasAllFlags(bufferDesc.miscFlag, GHL::EBufferMiscFlag::StructuredBuffer)) {
					// This is a Structured Buffer
					uavDesc.Format = DXGI_FORMAT_UNKNOWN;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)bufferDesc.size / bufferDesc.stride;
					uavDesc.Buffer.StructureByteStride = bufferDesc.stride;
				}
				else if (HasAllFlags(bufferDesc.miscFlag, GHL::EBufferMiscFlag::IndirectArgs)) {
					uavDesc.Format = DXGI_FORMAT_R32_UINT;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)bufferDesc.size / sizeof(uint32_t);
				}
			}
			else {
				// This is a Typed Buffer
				uint32_t stride = GHL::GetFormatStride(bufferDesc.format);
				uavDesc.Format = bufferDesc.format;
				uavDesc.Buffer.FirstElement = 0u;
				uavDesc.Buffer.NumElements = (UINT)bufferDesc.size / stride;
			}

			mUADescriptor = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDevice->D3DDevice()->CreateUnorderedAccessView(mD3DResource.Get(), nullptr, &uavDesc, *mUADescriptor.Get());
		}
	}
}