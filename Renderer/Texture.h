#pragma once
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "Buffer.h"

#include "Resource.h"

namespace Renderer {

	class Texture : public Renderer::Resource {
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

		// ==============================...流式纹理...==============================

		inline const auto& GetTileShape()     const { return mTileShape; }
		inline const auto& GetNumTilesTotal() const { return mNumTilesTotal; }
		inline const auto& GetTilings()       const { return mTiling; }

	private:
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mSRDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mDSDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mRTDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mUADescriptors;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation;

		// ==============================...流式纹理...==============================
		// 变量

		D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
		D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		UINT mNumTilesTotal;
		std::vector<D3D12_SUBRESOURCE_TILING> mTiling;

		Microsoft::WRL::ComPtr<ID3D12Resource2> mFeedbackResource;
		D3D12_RESOURCE_DESC1 mFeedbackDesc{};
	};

}