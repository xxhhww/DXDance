#pragma once
#include "Renderer/Resource.h"
#include "Renderer/Texture.h"
#include "Renderer/Buffer.h"
#include "Renderer/RingFrameTracker.h"

#include "Tools/Wrap.h"
#include "Tools/Pool.h"

namespace Renderer {

	using TextureWrap = Tool::Wrap<Renderer::Texture>;
	using BufferWrap  = Tool::Wrap<Renderer::Buffer>;

	class ResourceAllocator {
	public:
		struct SlotUserData {
			std::optional<uint64_t> resourceIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserData>;

		struct Deallocation {
			typename Pool::Slot* slot;
		};

	public:
		ResourceAllocator(RingFrameTracker* ringFrameTracker);
		~ResourceAllocator();

		TextureWrap Allocate(
			const GHL::Device* device,
			const TextureDesc& textureDesc,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);
		
		TextureWrap Allocate(
			const GHL::Device* device,
			const TextureDesc& textureDesc,
			PoolDescriptorAllocator* descriptorAllocator,
			const GHL::Heap* heap,
			size_t heapOffset);
		
		BufferWrap  Allocate(
			const GHL::Device* device,
			const BufferDesc& bufferDesc,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);

		BufferWrap Allocate(
			const GHL::Device* device,
			const BufferDesc& bufferDesc,
			PoolDescriptorAllocator* descriptorAllocator,
			const GHL::Heap* heap,
			size_t heapOffset);

	private:
		void CleanUpPendingDeallocation(uint8_t frameIndex);

	private:
		RingFrameTracker*  mFrameTracker{ nullptr };

		Pool mResourcePool;
		std::vector<std::unique_ptr<Renderer::Resource>> mResources;

		std::vector<std::vector<Deallocation>> mPendingDeallocations;
	};

}