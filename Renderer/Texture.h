#pragma once
#include "GHL/Device.h"
#include "GHL/Resource.h"

#include "TextureDesc.h"
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "Buffer.h"

namespace Renderer {

	class Texture : public GHL::Resource {
	public:

		/*
		* Committed: heapAllocator为空
		* Placed   : textureDesc中的reserved标记为false且heapAllocator不为空
		* Reserved : textureDesc中的reserved标记为true且heapAllocator不为空
		*/
		Texture(
			const GHL::Device* device,
			const TextureDesc& textureDesc,
			uint8_t backBufferCount,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator
		);

		Texture(
			const GHL::Device* device,
			const TextureDesc& textureDesc,
			uint8_t backBufferCount,
			PoolDescriptorAllocator* descriptorAllocator,
			const GHL::Heap* heap,
			size_t heapOffset
		);

		~Texture();

		/*
		* Get方法
		*/
		inline const auto& GetTextureDesc()  const { return mTextureDesc; }
		inline const auto* GetSRDescriptor() const { return mSRDescriptor.Get(); }
		inline const auto* GetUADescriptor() const { return mUADescriptor.Get(); }

		/*
		* 解算D3D12_RESOURCE_DESC
		*/
		void ResolveResourceDesc() override;

		/*
		* 创建描述符
		*/
		void CreateDescriptor() override;

	private:
		const GHL::Device* mDevice{ nullptr };
		TextureDesc mTextureDesc{};
		uint8_t mBackBufferCount;

		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		DescriptorHandleWrap mSRDescriptor;
		DescriptorHandleWrap mUADescriptor;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		std::vector<BuddyHeapAllocator::Allocation*> mHeapAllocations;

	};

}