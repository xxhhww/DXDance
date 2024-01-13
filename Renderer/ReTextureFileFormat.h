#pragma once
#include "GHL/pbh.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Renderer {

	class ReTextureFileFormat {
	public:
		struct FileHeader {
		public:
			uint32_t compressionFormat{ 1u };	// ����ѹ����ʽ
			
			uint32_t dxgiFormat;				// dxgi	
			uint32_t tileWidth;
			uint32_t tileHeight;
			uint32_t tileRowPitch;				// ע�⣡�����ÿһ�У��������һ�У�����Ҫ256�ֽڶ��룬����������ҲҪ256�ֽڶ���
			uint32_t tileSlicePitch;
			uint32_t tileNums;
			uint32_t subresourceNums;
			
			uint32_t arraySize;					// �������				
			uint32_t mipLevels;
			uint32_t imageWidth;
			uint32_t imageHeight;
		};

		struct SubresourceInfo {
		public:
			uint32_t widthTiles;
			uint32_t heightTiles;
			uint32_t depthTiles;

			// convenience value, can be computed from sum of previous subresource dimensions
			uint32_t subresourceTileIndex;
		};

		struct TileDataInfo {
		public:
			uint32_t offset;          // file offset to tile data
			uint32_t numBytes;        // # bytes for the tile(compressed)
		};

	public:
		ReTextureFileFormat(const std::string& filepath);
		~ReTextureFileFormat();

		inline const auto& GetFilePath() const { return mFilepath; }
		inline const auto& GetFileHeader() const { return mFileHeader; }
		inline const auto& GetSubresourceInfos() const { return mSubresourceInfos; }
		inline const auto& GetSubresourceNums() const { return mSubresourceInfos.size(); }
		inline const auto& GetTileDataInfos() const { return mTileDataInfos; }
		inline const auto& GetTileNums() const { return mTileDataInfos.size(); }
		inline const auto& GetTileByteSize() const { return GHL::GetFormatStride((DXGI_FORMAT)mFileHeader.dxgiFormat) * mFileHeader.tileWidth * mFileHeader.tileHeight; }

		const std::string GetFilename() const;

	private:
		std::string mFilepath;
		FileHeader mFileHeader;
		std::vector<SubresourceInfo> mSubresourceInfos;
		std::vector<TileDataInfo> mTileDataInfos;
	};

}