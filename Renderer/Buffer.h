#pragma once
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"

#include "Resource.h"

namespace Renderer {

	class Buffer : public Renderer::Resource {
	public:

		/*
		* 如果heapAllocator不为空，则Buffer使用Placed方式创建，Heap分配自heapAllocator，否则Buffer使用Committed方式创建
		*/
		Buffer(
			const GHL::Device* device,
			const ResourceFormat& resourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);

		/*
		* 使用Placed方式创建Buffer，Heap由外部提供
		*/
		Buffer(
			const GHL::Device* device,
			const ResourceFormat& resourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			const GHL::Heap* heap,
			size_t heapOffset
		);

		~Buffer();

		/*
		* 如果是Upload或者Readback的Buffer则将数据口映射到共享内存中，供CPU端读取与写入
		*/
		uint8_t* Map();

		/*
		* 关闭映射
		*/
		void UnMap();

		/*
		* 创建描述符
		*/
		void CreateDescriptor() override;

		inline const auto* GetSRDescriptor()   const { return mSRDescriptor.Get(); }
		inline const auto* GetUADescriptor()   const { return mUADescriptor.Get(); }

	private:
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		DescriptorHandleWrap mSRDescriptor;
		DescriptorHandleWrap mUADescriptor;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };

		uint8_t* mMappedMemory{ nullptr };

	};

}