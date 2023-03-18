#pragma once
#include "GHL/Resource.h"

#include "Buffer.h"
#include "Texture.h"
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "RingFrameTracker.h"

#include "Tools/Wrap.h"

namespace Renderer {

	using BufferWrap = Tool::Wrap<Buffer>;
	using TextureWrap = Tool::Wrap<Texture>;
	/*
	* 普通的资源分配器，负责资源的创建与延迟释放
	* PS：其他分配器作为池化分配器，仅负责对象的创建与重用，并不负责对象的释放，只有在整个程序关闭时才释放资源
	*/
	class ResourceAllocator {
	public:
		ResourceAllocator(
			const GHL::Device* device, 
			RingFrameTracker* frameTracker, 
			BuddyHeapAllocator* heapAllocator, 
			PoolDescriptorAllocator* descriptorAllocator);
		~ResourceAllocator();

	private:
		void CleanUpPendingDeallocation(uint8_t frameIndex);


	private:
		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::vector<GHL::Resource*> mPendingDeallocations;
	};

}