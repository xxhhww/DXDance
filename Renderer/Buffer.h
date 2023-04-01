#pragma once
#include "GHL/Resource.h"
#include "GHL/Heap.h"

#include "ResourceFormat.h"
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"

namespace Renderer {

	class Buffer : public GHL::Resource {
	public:

		/*
		* ���heapAllocator��Ϊ�գ���Bufferʹ��Placed��ʽ������Heap������heapAllocator������Bufferʹ��Committed��ʽ����
		*/
		Buffer(
			const GHL::Device* device,
			const ResourceFormat& resourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);

		/*
		* ʹ��Placed��ʽ����Buffer��Heap���ⲿ�ṩ
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
		* �����Upload����Readback��Buffer�����ݿ�ӳ�䵽�����ڴ��У���CPU�˶�ȡ��д��
		*/
		uint8_t* Map();

		/*
		* �ر�ӳ��
		*/
		void UnMap();

		/*
		* ����������
		*/
		void CreateDescriptor() override;

		inline const auto* GetDevice()         const { return mDevice; }
		inline const auto& GetResourceFormat() const { return mResourceFormat; }
		inline const auto* GetSRDescriptor()   const { return mSRDescriptor.Get(); }
		inline const auto* GetUADescriptor()   const { return mUADescriptor.Get(); }

	private:
		const GHL::Device* mDevice{ nullptr };

		ResourceFormat mResourceFormat;

		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		DescriptorHandleWrap mSRDescriptor;
		DescriptorHandleWrap mUADescriptor;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };

		uint8_t* mMappedMemory{ nullptr };

	};

}