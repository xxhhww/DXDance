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
	* ��ͨ����Դ��������������Դ�Ĵ������ӳ��ͷ�
	* PS��������������Ϊ�ػ������������������Ĵ��������ã��������������ͷţ�ֻ������������ر�ʱ���ͷ���Դ
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