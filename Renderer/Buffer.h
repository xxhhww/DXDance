#pragma once
#include "GHL/Resource.h"
#include "GHL/Heap.h"

#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "BufferDesc.h"

namespace Renderer {

	class Buffer : public GHL::Resource {
	public:

		Buffer(
			const GHL::Device* device,
			BufferDesc bufferDesc,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);

		Buffer(
			const GHL::Device* device,
			BufferDesc bufferDesc,
			PoolDescriptorAllocator* descriptor,
			const GHL::Heap* heap,
			size_t heapOffset
		);

		~Buffer();

		uint8_t* Map();

		void UnMap();

		void ResolveResourceDesc() override;

		void CreateDescriptor() override;

	private:
		const GHL::Device* mDevice{ nullptr };

		BufferDesc mBufferDesc{};

		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		DescriptorHandleWrap mSRDescriptor;
		DescriptorHandleWrap mUADescriptor;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };

		uint8_t* mMappedMemory{ nullptr };

	};

}