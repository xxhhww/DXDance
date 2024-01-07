#pragma once
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/Resource.h"
#include "DirectStorage/dstorage.h"

namespace Renderer {
	
	/*
	* ����ͼ����Դ����
	*/
	struct TextureAtlasFileDescriptor {
	public:
		/*
		* �ļ�ͷ��Ϣ
		*/
		struct FileHeader {
		public:
			uint32_t compressionFormat{ 1u };	// 0 is no compression
			DXGI_FORMAT dxgiFormat;
			uint32_t tileWidth;
			uint32_t tileHeight;
			uint32_t tileRowPitch;				// ע�⣡�����ÿһ�У��������һ�У�����Ҫ256�ֽڶ��룬����������ҲҪ256�ֽڶ���
			uint32_t tileSlicePitch;
			uint32_t tileCount;
		};

		/*
		* TileData����
		*/
		struct TileDataDescriptorInFile {
		public:
			uint32_t offset;					// file offset to tile data
			uint32_t numBytes;					// # bytes for the tile
		};

	public:
		TextureAtlasFileDescriptor(const std::string& filepath);
		~TextureAtlasFileDescriptor();

		inline const auto& GetFilePath() const { return mFilepath; }
		inline const auto& GetTileCount() const { return mFileHeader.tileCount; }
		inline const auto& GetFileHeader() const { return mFileHeader; }
		inline const auto& GetCompressForamt() const { return mFileHeader.compressionFormat; }
		inline const auto& GetTileDataDescriptors() const { return mTileDataDescriptors; }

		inline const auto& GetTileDataDescriptorsByIndex(uint32_t index) const { return mTileDataDescriptors.at(index); }

	private:
		std::string mFilepath;		// ����·��
		FileHeader mFileHeader;
		std::vector<TileDataDescriptorInFile> mTileDataDescriptors;
	};

	class TextureAtlas;
	class TextureAtlasTile {
	public:
		TextureAtlasTile(
			const GHL::Device* device,
			TextureAtlas* atlas,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator
		);

		~TextureAtlasTile();

		/*
		* ��Ĭ�϶�����Placed��ʽ������Դ
		*/
		void Create();

		/*
		* �ͷ���Դ��Ĭ�϶��ϵ��Դ����
		*/
		void Release();

		inline ID3D12Resource* D3DResource() const { return mD3DResource.Get(); }

	private:
		void CreateSRDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

	private:
		const GHL::Device* mDevice{ nullptr };
		TextureAtlas* mAtlas{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };

		Microsoft::WRL::ComPtr<ID3D12Resource> mD3DResource{ nullptr };
		
		PoolDescriptorAllocator::Allocation* mDescriptorAllocation{ nullptr };
		BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };
	};

	/*
	* ����ͼ��
	*/
	class TextureAtlas {
	public:
		TextureAtlas(
			const GHL::Device* device,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator,
			IDStorageFactory* dstorageFactory,
			const std::string& filepath
		);

		~TextureAtlas();

		inline const auto& GetSubresourceFormat()          const { return mSubResourceFormat; }
		inline const auto& GetTextureAtlasFileDescriptor() const { return mTextureAtlasFileDescriptor; }
		inline const auto& GetTileDataDescriptorInFile(uint32_t index) const { return mTextureAtlasFileDescriptor.GetTileDataDescriptorsByIndex(index); }

		inline auto* GetDSFileHandle()            const { return mDStorageFile.Get(); }
		inline auto* GetAtlasTile(uint32_t index) const { return mTextureAtlasTiles.at(index).get(); }

	private:
		const GHL::Device* mDevice{ nullptr };
		ResourceFormat mSubResourceFormat;
		TextureAtlasFileDescriptor mTextureAtlasFileDescriptor;
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageFile> mDStorageFile;

		std::vector<std::unique_ptr<TextureAtlasTile>> mTextureAtlasTiles;
	};
}