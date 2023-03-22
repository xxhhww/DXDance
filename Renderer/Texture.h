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
		* Committed: heapAllocatorΪ��
		* Placed   : textureDesc�е�reserved���Ϊfalse��heapAllocator��Ϊ��
		* Reserved : textureDesc�е�reserved���Ϊtrue��heapAllocator��Ϊ��
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
		* Get����
		*/
		inline const auto& GetTextureDesc()  const { return mTextureDesc; }
		
		const GHL::DescriptorHandle* GetDSDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});
		const GHL::DescriptorHandle* GetSRDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

		const GHL::DescriptorHandle* GetRTDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});
		const GHL::DescriptorHandle* GetUADescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

		/*
		* ����D3D12_RESOURCE_DESC
		*/
		void ResolveResourceDesc() override;

		/*
		* ����������
		*/
		void CreateDescriptor() override {}

		// ==============================...��ʽ����...==============================
		// ����

		/*
		* Get����
		*/
		inline const auto& GetTileShape()     const { return mTileShape; }
		inline const auto& GetNumTilesTotal() const { return mNumTilesTotal; }
		inline const auto& GetTilings()       const { return mTiling; }

	private:
		const GHL::Device* mDevice{ nullptr };
		TextureDesc mTextureDesc{};
		uint8_t mBackBufferCount;

		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::unordered_map<int, int> ss;

		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mSRDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mDSDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mRTDescriptors;
		std::unordered_map<TextureSubResourceDesc, DescriptorHandleWrap, TextureSubResourceDescHashFunc> mUADescriptors;

		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation;

		// ==============================...��ʽ����...==============================
		// ����

		D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
		D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		UINT mNumTilesTotal;
		std::vector<D3D12_SUBRESOURCE_TILING> mTiling;

		Microsoft::WRL::ComPtr<ID3D12Resource2> mFeedbackResource;
		D3D12_RESOURCE_DESC1 mFeedbackDesc{};

	};

}