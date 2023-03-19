#pragma once
#include "GHL/Resource.h"
#include "GHL/Heap.h"

#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "BufferDesc.h"

namespace Renderer {

	class Buffer : public GHL::Resource {
	public:

		/*
		* ���heapAllocator��Ϊ�գ���Bufferʹ��Placed��ʽ������Heap������heapAllocator������Bufferʹ��Committed��ʽ����
		*/
		Buffer(
			const GHL::Device* device,
			const BufferDesc& bufferDesc,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);

		/*
		* ʹ��Placed��ʽ����Buffer��Heap���ⲿ�ṩ
		*/
		Buffer(
			const GHL::Device* device,
			const BufferDesc& bufferDesc,
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
		* ����D3D12_RESOURCE_DESC
		*/
		void ResolveResourceDesc() override;

		/*
		* ����������
		*/
		void CreateDescriptor() override;

		/*
		* Get����
		*/
		inline const auto& GetBufferDesc()   const { return mBufferDesc; }
		inline const auto* GetSRDescriptor() const { return mSRDescriptor.Get(); }
		inline const auto* GetUADescriptor() const { return mUADescriptor.Get(); }

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