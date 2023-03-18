#include "Buffer.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	Buffer::Buffer(
		const GHL::Device* device,
		BufferDesc bufferDesc,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator
		) 
	: mDevice(device)
	, mBufferDesc(bufferDesc)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {

		ResolveResourceDesc();

		// 设置资源的初始状态
		D3D12_HEAP_PROPERTIES heapProperties{};
		if (mBufferDesc.usage == GHL::EResourceUsage::Default) {
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::RaytracingAccelerationStructure)) {
				mInitialStates = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}
			else if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::IndirectArgument)) {
				mInitialStates = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
			}
		}
		else if (mBufferDesc.usage == GHL::EResourceUsage::Upload) {
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			mInitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (mBufferDesc.usage == GHL::EResourceUsage::ReadBack) {
			heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
			mInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else {
			ASSERT_FORMAT(false, "Unsupport Resource Usage");
		}

		// 上传资源与回读资源默认使用Committed方式创建
		if (mBufferDesc.usage == GHL::EResourceUsage::Upload ||
			mBufferDesc.usage == GHL::EResourceUsage::ReadBack ||
			(mBufferDesc.usage == GHL::EResourceUsage::Default && heapAllocator == nullptr)) {
			// 以默认方式创建该Buffer
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mResourceDesc,
				mInitialStates,
				nullptr,
				IID_PPV_ARGS(&mResource)
			));
		}
		else {
			// 在堆上分配
			mHeapAllocation = mHeapAllocator->Allocate(mBufferDesc.size);

			// 以放置方式创建该Buffer
			HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
				mHeapAllocation->heap->D3DHeap(),
				mHeapAllocation->heapOffset,
				&mResourceDesc,
				mInitialStates,
				nullptr,
				IID_PPV_ARGS(&mResource)
			));
		}

		CreateDescriptor();
	}

	Buffer::Buffer(
		const GHL::Device* device,
		BufferDesc bufferDesc,
		PoolDescriptorAllocator* descriptor,
		const GHL::Heap* heap,
		size_t heapOffset) {

		ResolveResourceDesc();

		// 设置资源的初始状态
		if (mBufferDesc.usage == GHL::EResourceUsage::Default) {
			if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::RaytracingAccelerationStructure)) {
				mInitialStates = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}
			else if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::IndirectArgument)) {
				mInitialStates = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
			}
		}
		else if (mBufferDesc.usage == GHL::EResourceUsage::Upload) {
			mInitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (mBufferDesc.usage == GHL::EResourceUsage::ReadBack) {
			mInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else {
			ASSERT_FORMAT(false, "Unsupport Resource Usage");
		}

		// 在堆上创建资源
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			heap->D3DHeap(),
			heapOffset,
			&mResourceDesc,
			mInitialStates,
			nullptr,
			IID_PPV_ARGS(&mResource)
		));
	}

	Buffer::~Buffer() {
		// TODO
	}

	uint8_t* Buffer::Map() {
		if (mMappedMemory) {
			return mMappedMemory;
		}

		D3D12_RANGE mapRange{ 0, mBufferDesc.size };
		HRASSERT(mResource->Map(0, &mapRange, (void**)&mMappedMemory));
		return mMappedMemory;
	}

	void Buffer::UnMap() {
		if (!mMappedMemory) {
			return;
		}

		mResource->Unmap(0, nullptr);
		mMappedMemory = nullptr;
	}

	void Buffer::ResolveResourceDesc() {
		if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::ConstantBuffer)) {
			// ConstantBuffer需要字节对齐
			mBufferDesc.size = Math::AlignUp(mBufferDesc.size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}

		mResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		mResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		mResourceDesc.MipLevels = 1u;
		mResourceDesc.Alignment = 0u;
		mResourceDesc.DepthOrArraySize = 1u;
		mResourceDesc.Height = 1u;
		mResourceDesc.Width = mBufferDesc.size;
		mResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		mResourceDesc.SampleDesc.Count = 1u;
		mResourceDesc.SampleDesc.Quality = 0u;

		if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::UnorderedAccess)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::PixelShaderAccess) &&
			!HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
	}

	void Buffer::CreateDescriptor() {
		// 创建描述符
		if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::UnorderedAccess)) {
			// UAView
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;

			if (mBufferDesc.format == DXGI_FORMAT_UNKNOWN) {
				if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::RawBuffer)) {
					// This is a Raw Buffer
					uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / sizeof(uint32_t);
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::StructuredBuffer)) {
					// This is a Structured Buffer
					uavDesc.Format = DXGI_FORMAT_UNKNOWN;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / mBufferDesc.stride;
					uavDesc.Buffer.StructureByteStride = mBufferDesc.stride;
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::IndirectArgs)) {
					uavDesc.Format = DXGI_FORMAT_R32_UINT;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / sizeof(uint32_t);
				}
			}
			else {
				// This is a Typed Buffer
				uint32_t stride = GHL::GetFormatStride(mBufferDesc.format);
				uavDesc.Format = mBufferDesc.format;
				uavDesc.Buffer.FirstElement = 0u;
				uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / stride;
			}

			mUADescriptor = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDevice->D3DDevice()->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, *mUADescriptor.Get());
		}

		if (HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::PixelShaderAccess) ||
			HasAllFlags(mBufferDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess)) {
			// SRView
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			bool isAccelStruct = false;
			if (mBufferDesc.format == DXGI_FORMAT_UNKNOWN) {
				if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::RawBuffer)) {
					// This is a Raw Buffer
					srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					srvDesc.Buffer.FirstElement = 0u;
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					srvDesc.Buffer.NumElements = (UINT)mBufferDesc.size / sizeof(uint32_t);
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::StructuredBuffer)) {
					// This is a Structured Buffer
					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.Buffer.FirstElement = 0u;
					srvDesc.Buffer.NumElements = (UINT)mBufferDesc.size / mBufferDesc.stride;
					srvDesc.Buffer.StructureByteStride = mBufferDesc.stride;
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, GHL::EBufferMiscFlag::AccelerateStruct)) {
					isAccelStruct = true;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
					srvDesc.RaytracingAccelerationStructure.Location = mResource->GetGPUVirtualAddress();
				}
			}
			else {
				// This is a Typed Buffer
				uint32_t stride = GHL::GetFormatStride(mBufferDesc.format);
				srvDesc.Format = mBufferDesc.format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = 0u;
				srvDesc.Buffer.NumElements = (UINT)mBufferDesc.size / stride;
			}

			mSRDescriptor = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDevice->D3DDevice()->CreateShaderResourceView(mResource.Get(), &srvDesc, *mSRDescriptor.Get());
		}
	}
}