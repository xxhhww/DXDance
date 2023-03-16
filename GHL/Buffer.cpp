#include "Buffer.h"
#include "Tools/Assert.h"
#include "Math/Helper.h"

namespace GHL {
	/*
	Buffer::Buffer(GpuDevice* gpuDevice, const BufferDesc& bufferDesc)
	: Resource(gpuDevice)
	, mBufferDesc(bufferDesc) {
		if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::ConstantBuffer)) {
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

		if (HasAllFlags(mBufferDesc.expectedState, EResourceState::UnorderedAccess)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!HasAllFlags(mBufferDesc.expectedState, EResourceState::PixelShaderAccess) &&
			!HasAllFlags(mBufferDesc.expectedState, EResourceState::NonPixelShaderAccess)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		// 设置资源的初始状态
		D3D12_HEAP_PROPERTIES heapProperties;
		if (mBufferDesc.usage == EResourceUsage::Default) {
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			if (HasAllFlags(mBufferDesc.expectedState, EResourceState::RaytracingAccelerationStructure)) {
				mInitialStates = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}
			else if (HasAllFlags(mBufferDesc.expectedState, EResourceState::IndirectArgument)) {
				mInitialStates = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
			}
		}
		else if (mBufferDesc.usage == EResourceUsage::Upload) {
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			mInitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (mBufferDesc.usage == EResourceUsage::ReadBack) {
			heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
			mInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else {
			ASSERT_FORMAT(false, "Unsupport Resource Usage");
		}

		if (!mBufferDesc.placed) {
			// 以默认方式创建该Buffer
			mGpuDevice->device.D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mResourceDesc,
				mInitialStates,
				nullptr,
				IID_PPV_ARGS(&mResource)
			);
		}

		// 创建描述符
		if (HasAllFlags(mBufferDesc.expectedState, EResourceState::UnorderedAccess)) {
			// UAView
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;

			if (mBufferDesc.format == DXGI_FORMAT_UNKNOWN) {
				if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::RawBuffer)) {
					// This is a Raw Buffer
					uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / sizeof(uint32_t);
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::StructuredBuffer)) {
					// This is a Structured Buffer
					uavDesc.Format = DXGI_FORMAT_UNKNOWN;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / mBufferDesc.stride;
					uavDesc.Buffer.StructureByteStride = mBufferDesc.stride;
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::IndirectArgs)) {
					uavDesc.Format = DXGI_FORMAT_R32_UINT;
					uavDesc.Buffer.FirstElement = 0u;
					uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / sizeof(uint32_t);
				}
			}
			else {
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(mBufferDesc.format);
				uavDesc.Format = mBufferDesc.format;
				uavDesc.Buffer.FirstElement = 0u;
				uavDesc.Buffer.NumElements = (UINT)mBufferDesc.size / stride;
			}

			mUADescriptor = mGpuDevice->descriptorAllocator.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mGpuDevice->device.D3DDevice()->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, mUADescriptor);
		}

		if (HasAllFlags(mBufferDesc.expectedState, EResourceState::PixelShaderAccess) ||
			HasAllFlags(mBufferDesc.expectedState, EResourceState::NonPixelShaderAccess)) {
			// SRView
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			bool isAccelStruct = false;
			if (mBufferDesc.format == DXGI_FORMAT_UNKNOWN) {
				if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::RawBuffer)) {
					// This is a Raw Buffer
					srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					srvDesc.Buffer.FirstElement = 0u;
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					srvDesc.Buffer.NumElements = (UINT)mBufferDesc.size / sizeof(uint32_t);
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::StructuredBuffer)) {
					// This is a Structured Buffer
					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.Buffer.FirstElement = 0u;
					srvDesc.Buffer.NumElements = (UINT)mBufferDesc.size / mBufferDesc.stride;
					srvDesc.Buffer.StructureByteStride = mBufferDesc.stride;
				}
				else if (HasAllFlags(mBufferDesc.miscFlag, EBufferMiscFlag::AccelerateStruct)) {
					isAccelStruct = true;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
					srvDesc.RaytracingAccelerationStructure.Location = mResource->GetGPUVirtualAddress();
				}
			}
			else {
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(mBufferDesc.format);
				srvDesc.Format = mBufferDesc.format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = 0u;
				srvDesc.Buffer.NumElements = (UINT)mBufferDesc.size / stride;
			}

			mSRDescriptor = mGpuDevice->descriptorAllocator.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mGpuDevice->device.D3DDevice()->CreateShaderResourceView(mResource.Get(), &srvDesc, mSRDescriptor);
		}
	}

	uint8_t* Buffer::Map() {
		if (mMappedMemory) {
			return mMappedMemory;
		}

		D3D12_RANGE mapRange{ 0, mBufferDesc.size };
		HRASSERT(mResource->Map(0, &mapRange, (void**)&mMappedMemory));
		return mMappedMemory;
	}

	void Buffer::Unmap() {
		if (!mMappedMemory) {
			return;
		}

		mResource->Unmap(0, nullptr);
		mMappedMemory = nullptr;
	}

	const D3D12_VERTEX_BUFFER_VIEW Buffer::GetVBView() {
		return D3D12_VERTEX_BUFFER_VIEW{
			mResource->GetGPUVirtualAddress(),
			(UINT)mBufferDesc.size,
			(UINT)mBufferDesc.stride
		};
	}

	const D3D12_INDEX_BUFFER_VIEW  Buffer::GetIBView() {
		return D3D12_INDEX_BUFFER_VIEW{
			mResource->GetGPUVirtualAddress(),
			(UINT)mBufferDesc.size,
			mBufferDesc.stride == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
		};
	}
	*/
}