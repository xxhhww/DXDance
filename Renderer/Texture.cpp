#include "Texture.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	Texture::Texture(
		const GHL::Device* device,
		const TextureDesc& textureDesc,
		uint8_t backBufferCount,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator
	)
	: mDevice(device)
	, mTextureDesc(textureDesc)
	, mBackBufferCount(backBufferCount)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		
		if (mTextureDesc.reserved) {
			ASSERT_FORMAT(mHeapAllocator != nullptr, "Texture Created Reserved, HeapAllocator Is Nullptr");
		}


	}

	Texture::Texture(
		const GHL::Device* device,
		const TextureDesc& textureDesc,
		uint8_t backBufferCount,
		PoolDescriptorAllocator* descriptorAllocator,
		const GHL::Heap* heap,
		size_t heapOffset
	)
	: mDevice(device)
	, mTextureDesc(textureDesc)
	, mBackBufferCount(backBufferCount)
	, mDescriptorAllocator(descriptorAllocator) {

	}

	Texture::~Texture() {

	}

	void Texture::ResolveResourceDesc() {

	}

	void Texture::CreateDescriptor() {

	}

}