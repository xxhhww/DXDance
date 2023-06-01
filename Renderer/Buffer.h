#pragma once
#include "Mesh.h"
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"

#include "Resource.h"

namespace Renderer {

	class Buffer : public Renderer::Resource {
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
		void CreateDescriptor() override {}

		const GHL::DescriptorHandle* GetSRDescriptor(const BufferSubResourceDesc& subDesc = BufferSubResourceDesc{});
		const GHL::DescriptorHandle* GetUADescriptor(const BufferSubResourceDesc& subDesc = BufferSubResourceDesc{});

		inline const auto GetVBDescriptor() const {
			D3D12_VERTEX_BUFFER_VIEW vbView{};
			vbView.BufferLocation = mD3DResource->GetGPUVirtualAddress();
			vbView.StrideInBytes = sizeof(Renderer::Vertex);
			vbView.SizeInBytes = mResourceFormat.GetSizeInBytes();
			return vbView;
		}

		inline const auto GetIBDescriptor() const {
			D3D12_INDEX_BUFFER_VIEW ibView{};
			ibView.BufferLocation = mD3DResource->GetGPUVirtualAddress();
			ibView.Format = DXGI_FORMAT_R32_UINT;
			ibView.SizeInBytes = mResourceFormat.GetSizeInBytes();
			return ibView;
		}

		inline Buffer* GetCounterBuffer() const { return (mCounterBuffer == nullptr) ? nullptr : mCounterBuffer.get(); }

	private:
		void CreateCounterBuffer();

	private:
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::unordered_map<BufferSubResourceDesc, DescriptorHandleWrap, BufferSubResourceDescHashFunc> mSRDescriptors;
		std::unordered_map<BufferSubResourceDesc, DescriptorHandleWrap, BufferSubResourceDescHashFunc> mUADescriptors;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };

		uint8_t* mMappedMemory{ nullptr };

		std::unique_ptr<Buffer> mCounterBuffer;

	};

}