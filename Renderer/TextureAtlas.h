#pragma once
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/Resource.h"
#include "DirectStorage/dstorage.h"

namespace Renderer {

	class TextureAtlasTile {
	public:
		TextureAtlasTile(
			const GHL::Device* device,
			const ResourceFormat& subResourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator
		);

		~TextureAtlasTile();

		/*
		* 在默认堆上以Placed方式创建资源
		*/
		void Create();

		/*
		* 释放资源在默认堆上的显存分配
		*/
		void Release();

		inline ID3D12Resource* D3DResource() const { return mD3DResource.Get(); }

	private:
		void CreateSRDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

	private:
		const GHL::Device* mDevice{ nullptr };
		ResourceFormat mResourceFormat{};
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };

		Microsoft::WRL::ComPtr<ID3D12Resource> mD3DResource{ nullptr };
		
		PoolDescriptorAllocator::Allocation* mDescriptorAllocation{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };
	};

	/*
	* 纹理图集资源描述
	*/
	struct TextureAtlasFileDescriptor {
	public:
		/*
		* 文件头信息
		*/
		struct FileHeader {
		public:
			uint32_t compressionFormat{ 1u };	// 0 is no compression
			DXGI_FORMAT dxgiFormat;
			uint32_t tileWidth;
			uint32_t tileHeight;
			uint32_t tileRowPitch;				// 注意！纹理的每一行（除了最后一行）都需要256字节对齐，并且纹理本身也要256字节对齐
			uint32_t tileSlicePitch;
			uint32_t tileCount;
		};

		/*
		* TileData描述
		*/
		struct TileDataDescriptor {
		public:
			uint32_t offset;					// file offset to tile data
			uint32_t numBytes;					// # bytes for the tile
		};

	public:
		TextureAtlasFileDescriptor(const std::string& filepath);
		~TextureAtlasFileDescriptor() = default;

		const FileHeader& GetFileHeader() const { return mFileHeader; }

	private:
		std::string mFilepath;		// 绝对路径
		FileHeader mFileHeader;
		std::vector<TileDataDescriptor> mTileDataDescriptors;
	};

	/*
	* 纹理图集
	*/
	class TextureAtlas {
	public:
		TextureAtlas(
			const GHL::Device* device,
			const ResourceFormat& subResourceFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator,
			IDStorageFactory* dstorageFactory,
			const std::string& filepath
		);

		~TextureAtlas();

		inline const auto& GetSubresourceFormat()  const { return mSubResourceFormat; }
		inline const auto& GetTextureAtlasFileDescriptor() const { return mTextureAtlasFileDescriptor; }

		inline auto* GetDSFileHandle()              const { return mDStorageFile.Get(); }
		inline auto* GetSubresource(uint32_t index) const { return mTextureAtlasTiles.at(index).get(); }

	private:
		const GHL::Device* mDevice{ nullptr };
		ResourceFormat mSubResourceFormat;
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		std::unique_ptr<TextureAtlasFileDescriptor> mTextureAtlasFileDescriptor;
		Microsoft::WRL::ComPtr<IDStorageFile> mDStorageFile;

		std::vector<std::unique_ptr<TextureAtlasTile>> mTextureAtlasTiles;
	};
}