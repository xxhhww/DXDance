#pragma once
#include "Resource.h"
#include "ResourceDesc.h"
#include "DescriptorHeap.h"

namespace GHL {
	/*
	class GpuDevice;

	class Buffer : public Resource {
	public:
		Buffer(GpuDevice* gpuDevice, const BufferDesc& bufferDesc);
		~Buffer() = default;

		uint8_t* Map();

		void Unmap();

		inline const auto& GetSRView() const { return mSRDescriptor; }
		inline const auto& GetUAView() const { return mUADescriptor; }
		inline const auto& GetBufferDesc()   const { return mBufferDesc; }

		const D3D12_VERTEX_BUFFER_VIEW GetVBView();
		const D3D12_INDEX_BUFFER_VIEW  GetIBView();

	private:
		GpuDevice* mGpuDevice{ nullptr };
		BufferDesc mBufferDesc{};
		uint8_t* mMappedMemory{ nullptr };

		DescriptorHandle mSRDescriptor;
		DescriptorHandle mUADescriptor;
	};
	*/
}