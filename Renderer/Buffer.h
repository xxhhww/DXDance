#pragma once
#include "Mesh.h"
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