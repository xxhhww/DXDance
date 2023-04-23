#pragma once
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "Buffer.h"

#include "Resource.h"

namespace Renderer {

	class StreamTexture;

	class Texture : public Renderer::Resource {
		friend class StreamTexture;
	public:
		/*
		* Committed: heapAllocator为空
		* Placed   : textureDesc中的reserved标记为false且heapAllocator不为空
		* Reserved : textureDesc中的reserved标记为true且heapAllocator不为空
		*/
		Texture(
			const GHL::Device* device,
			const ResourceFormat& resourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator
		);

		Texture(
			const GHL::Device* device,
			const ResourceFormat& resourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			const GHL::Heap* heap,
			size_t heapOffset
		);

		Texture(
			const GHL::Device* device, 
			ID3D12Resource* backBuffer,
			PoolDescriptorAllocator* descriptorAllocator);

		~Texture();
		
		const GHL::DescriptorHandle* GetDSDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});
		const GHL::DescriptorHandle* GetSRDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});
		void BindSRDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

		const GHL::DescriptorHandle* GetRTDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});
		const GHL::DescriptorHandle* GetUADescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

		/*
		* 创建描述符
		*/
		void CreateDescriptor() override {}

	protected:
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mSRDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mDSDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mRTDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mUADescriptors;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation;
	};

}