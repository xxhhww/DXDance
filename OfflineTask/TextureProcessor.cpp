#include "OfflineTask/TextureProcessor.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"
#include "Math/Helper.h"
#include "GHL/d3dx12.h"
#include "Renderer/ReTextureFileFormat.h"

#include <wincodec.h>
#include <fstream>
#include <filesystem>
#include <random>
#include <opencv2/opencv.hpp>
#include <DirectStorage/dstorage.h>
#include <DirectXTex/DirectXTex.h>
#include <wrl/client.h>
#include <dxgi.h>

namespace OfflineTask {

	void TextureProcessor::Padding(const std::string& filename, const std::string& dirname, uint32_t paddingSize) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		// 读取纹理数据至内存
		DirectX::ScratchImage srcImage;
		HRASSERT(DirectX::LoadFromWICFile(
			Tool::StrUtil::UTF8ToWString(filename).c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			srcImage
		));

		uint32_t srcWidth = srcImage.GetImages()[0].width;
		uint32_t srcHeight = srcImage.GetImages()[0].height;
		uint32_t elementByteSize = srcImage.GetImages()[0].rowPitch / srcImage.GetImages()[0].width;

		// 扩展后的高度图
		DirectX::Image dstImage;
		dstImage.width = srcWidth + paddingSize;
		dstImage.height = srcHeight + paddingSize;
		dstImage.format = srcImage.GetImages()[0].format;
		dstImage.rowPitch = dstImage.width * elementByteSize;
		dstImage.slicePitch = dstImage.width * dstImage.height * elementByteSize;
		dstImage.pixels = new uint8_t[dstImage.slicePitch];

		for (uint32_t row = 0; row < dstImage.height; row++) {
			uint64_t srcOffset = (row < srcHeight) ? row * srcImage.GetImages()[0].rowPitch : (srcHeight - 1) * srcImage.GetImages()[0].rowPitch;
			uint64_t dstOffset = row * dstImage.rowPitch;

			memcpy(dstImage.pixels + dstOffset, srcImage.GetImages()[0].pixels + srcOffset, srcImage.GetImages()[0].rowPitch);

			srcOffset = srcOffset + (srcImage.GetImages()[0].rowPitch - elementByteSize * paddingSize);
			dstOffset = dstOffset + srcImage.GetImages()[0].rowPitch;
			memcpy(dstImage.pixels + dstOffset, srcImage.GetImages()[0].pixels + srcOffset, elementByteSize * paddingSize);
		}

		// 数据复制完成，存入磁盘(V轴命名需翻转)
		std::string result = dirname + Tool::StrUtil::GetFilename(filename) + ".png";;
		HRASSERT(DirectX::SaveToWICFile(
			dstImage,
			DirectX::WIC_FLAGS_NONE,
			DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
			Tool::StrUtil::UTF8ToWString(result).c_str()
		));
	}

	void TextureProcessor::Split(const std::string& filename, const std::string& dirname, uint32_t subSize, uint32_t step, uint32_t startNameIndex) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		// 读取纹理数据至内存
		DirectX::ScratchImage srcImage;
		HRASSERT(DirectX::LoadFromWICFile(
			Tool::StrUtil::UTF8ToWString(filename).c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			srcImage
		));

		uint32_t srcWidth = srcImage.GetImages()[0].width;
		uint32_t srcHeight = srcImage.GetImages()[0].height;
		ASSERT_FORMAT(srcWidth == srcHeight, "Muse Equal");

		uint32_t elementByteSize = srcImage.GetImages()[0].rowPitch / srcImage.GetImages()[0].width;

		// 切分后的图片的存储位置
		DirectX::Image dstImage;
		dstImage.width = subSize;
		dstImage.height = subSize;
		dstImage.format = srcImage.GetImages()[0].format;
		dstImage.rowPitch = dstImage.width * elementByteSize;
		dstImage.slicePitch = dstImage.width * dstImage.height * elementByteSize;
		dstImage.pixels = new uint8_t[dstImage.slicePitch];

		uint32_t uid = startNameIndex;
		for (uint32_t j = 0; j < srcHeight - 1; j += step) {
			for (uint32_t i = 0; i < srcWidth - 1; i += step) {
				// 计算切分后图像相对于原始图形的起始偏移量
				uint64_t srcOffset = j * srcImage.GetImages()[0].rowPitch + i * elementByteSize;
				uint64_t dstOffset = 0u;

				for (uint32_t index = 0; index < dstImage.height; index++) {
					memcpy(dstImage.pixels + dstOffset, srcImage.GetImages()[0].pixels + srcOffset, dstImage.rowPitch);

					srcOffset += srcImage.GetImages()[0].rowPitch;
					dstOffset += dstImage.rowPitch;
				}
				/*
				uint32_t binOffset = 0u;
				std::vector<float> heightDatas;
				for (uint32_t index = 0; index < dstImage.height; index++) {
					for (uint32_t groupIndex = 0; groupIndex < dstImage.width; groupIndex++) {
						// 读取一个组下的RGBA值
						uint16_t u16R = 0;
						uint16_t u16G = 0;
						uint16_t u16B = 0;
						uint16_t u16A = 0;
						memcpy(&u16R, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
						memcpy(&u16G, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
						memcpy(&u16B, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
						memcpy(&u16A, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;

						assert((u16R == u16G) && (u16G == u16B) && u16A == 65535);
						float heightData = (float)u16R / (float)u16A;
						heightDatas.push_back(heightData);
					}
				}

				std::string binFilename = "HeightMap_" + std::to_string(i / step) + "_" + std::to_string(j / step) + ".bin";
				std::ofstream fout(binFilename.c_str(), std::ios::binary | std::ios::out);
				fout.write((char*)heightDatas.data(), heightDatas.size() * sizeof(float));
				fout.close();
				*/

				// 数据复制完成，存入磁盘(V轴命名需翻转)
				std::string result = dirname + Tool::StrUtil::GetFilename(filename) + std::to_string(uid++) + ".png";
				HRASSERT(DirectX::SaveToWICFile(
					dstImage,
					DirectX::WIC_FLAGS_NONE,
					DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
					Tool::StrUtil::UTF8ToWString(result).c_str()
				));
			}
		}
	}

	void TextureProcessor::Resize(const std::string& filename, const std::string& dirname, uint32_t targetWidth, uint32_t targetHeight) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		// 读取纹理数据至内存
		DirectX::ScratchImage srcImage;
		HRASSERT(DirectX::LoadFromWICFile(
			Tool::StrUtil::UTF8ToWString(filename).c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			srcImage
		));

		uint32_t srcWidth = srcImage.GetImages()[0].width;
		uint32_t srcHeight = srcImage.GetImages()[0].height;
		uint8_t* pixel = srcImage.GetImages()[0].pixels;

		if (srcWidth == targetWidth && srcHeight == targetHeight) {
			HRASSERT(DirectX::SaveToWICFile(
				srcImage.GetImages()[0],
				DirectX::WIC_FLAGS_NONE,
				DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
				Tool::StrUtil::UTF8ToWString(dirname + Tool::StrUtil::GetFilename(filename) + ".png").c_str()
			));
			return;
		}

		DirectX::ScratchImage dstImage;
		HRASSERT(DirectX::Resize(srcImage.GetImages()[0], targetWidth, targetHeight, DirectX::TEX_FILTER_CUBIC, dstImage));

		// 保存
		HRASSERT(DirectX::SaveToWICFile(
			dstImage.GetImages()[0],
			DirectX::WIC_FLAGS_NONE,
			DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
			Tool::StrUtil::UTF8ToWString(dirname + Tool::StrUtil::GetFilename(filename) + ".png").c_str()
		));
	}

	void TextureProcessor::GenerateTextureAtlasFile1(const std::string& srcDirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& dstFilename) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		Microsoft::WRL::ComPtr<IDStorageCompressionCodec> compressor;
		DSTORAGE_COMPRESSION compressionLevel = DSTORAGE_COMPRESSION_BEST_RATIO;
		uint32_t compressionFormat = 1u;
		HRASSERT(DStorageCreateCompressionCodec((DSTORAGE_COMPRESSION_FORMAT)compressionFormat, 2, IID_PPV_ARGS(&compressor)),
			"DStorageCreateCompressionCodec Failed");

		Renderer::ReTextureFileFormat::FileHeader retFileHeader;
		std::vector<Renderer::ReTextureFileFormat::SubresourceInfo> retSubresourceInfos;
		std::vector<Renderer::ReTextureFileFormat::TileDataInfo> retTileDataInfos;
		std::vector<uint8_t> textureData;

		auto compressTileData = [&](std::vector<uint8_t>& tileData, size_t uncompressedDataSize) -> uint32_t {
			return tileData.size();

			auto bound = compressor->CompressBufferBound(uncompressedDataSize);
			std::vector<uint8_t> scratch(bound);

			size_t compressedDataSize = 0;
			HRESULT hr = compressor->CompressBuffer(tileData.data(), uncompressedDataSize, compressionLevel, scratch.data(), bound, &compressedDataSize);
			ASSERT_FORMAT(compressedDataSize <= uncompressedDataSize);

			scratch.resize(compressedDataSize);
			tileData.swap(scratch);

			return compressedDataSize;
		};

		bool firstImage = true;
		uint32_t entryNums = 0;
		for (uint32_t i = startIndex; i <= endIndex; i++) {
			std::string srcFilename = srcDirname + baseFilename + std::to_string(i) + ".png";

			entryNums++;
			DirectX::ScratchImage srcImage;
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(srcFilename).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				srcImage));

			// 注意，CPU端的图像的RowPitch、SlicePitch与GPU端的存储是不同的，因此需要特别注意
			uint32_t srcWidth = srcImage.GetImages()[0].width;
			uint32_t srcHeight = srcImage.GetImages()[0].height;
			uint32_t srcRowPitch = srcImage.GetImages()[0].rowPitch;
			uint32_t srcSlicePitch = srcImage.GetImages()[0].slicePitch;
			DXGI_FORMAT srcDxgiFormat = srcImage.GetImages()[0].format;
			uint8_t* srcData = srcImage.GetImages()[0].pixels;

			// uint32_t tileRowPitch = Math::AlignUp(srcRowPitch, 256);
			// uint32_t tileSlicePitch = Math::AlignUp(tileRowPitch * (srcHeight - 1) + srcRowPitch, 256);
			uint32_t tileRowPitch = srcRowPitch;
			uint32_t tileSlicePitch = srcSlicePitch;
			std::vector<uint8_t> tileData(tileSlicePitch);

			if (!firstImage) {
				ASSERT_FORMAT(retFileHeader.dxgiFormat == srcDxgiFormat);
				ASSERT_FORMAT(retFileHeader.tileWidth == srcWidth);
				ASSERT_FORMAT(retFileHeader.tileHeight == srcHeight);
				ASSERT_FORMAT(retFileHeader.tileRowPitch == tileRowPitch);
				ASSERT_FORMAT(retFileHeader.tileSlicePitch == tileSlicePitch);
			}
			else {
				retFileHeader.dxgiFormat = srcDxgiFormat;
				retFileHeader.tileWidth = srcWidth;
				retFileHeader.tileHeight = srcHeight;
				retFileHeader.tileRowPitch = tileRowPitch;
				retFileHeader.tileSlicePitch = tileSlicePitch;
				firstImage = false;
			}

			for (uint32_t row = 0; row < srcHeight; row++) {
				uint32_t srcOffset = row * srcRowPitch;
				uint32_t dstOffset = row * tileRowPitch;
				memcpy(tileData.data() + dstOffset, srcData + srcOffset, srcRowPitch);
			}


			uint32_t textureDataOffset = textureData.size();
			uint32_t compressedTileDataSize = compressTileData(tileData, tileSlicePitch);

			Renderer::ReTextureFileFormat::SubresourceInfo subresourceInfo;
			subresourceInfo.widthTiles = 1u;
			subresourceInfo.heightTiles = 1u;
			subresourceInfo.depthTiles = 1u;
			subresourceInfo.subresourceTileIndex = entryNums - 1;
			retSubresourceInfos.push_back(subresourceInfo);

			Renderer::ReTextureFileFormat::TileDataInfo tileDataInfo;
			tileDataInfo.offset = textureDataOffset;
			tileDataInfo.numBytes = compressedTileDataSize;
			retTileDataInfos.push_back(tileDataInfo);

			textureData.resize(textureData.size() + compressedTileDataSize);
			memcpy(textureData.data() + textureDataOffset, tileData.data(), compressedTileDataSize);
		}
		retFileHeader.tileNums = entryNums;
		retFileHeader.subresourceNums = retSubresourceInfos.size();
		retFileHeader.arraySize = 1u;
		retFileHeader.mipLevels = 1u;

		UINT64 textureDataOffsetInFile = sizeof(retFileHeader) +
			(retSubresourceInfos.size() * sizeof(Renderer::ReTextureFileFormat::SubresourceInfo)) +
			(retTileDataInfos.size() * sizeof(Renderer::ReTextureFileFormat::TileDataInfo));

		for (auto& descriptor : retTileDataInfos) {
			descriptor.offset += textureDataOffsetInFile;
		}

		std::ofstream outFile(dstFilename, std::ios::out | std::ios::binary);

		outFile.write((char*)&retFileHeader, sizeof(Renderer::ReTextureFileFormat::FileHeader));
		outFile.write((char*)retSubresourceInfos.data(), retSubresourceInfos.size() * sizeof(Renderer::ReTextureFileFormat::SubresourceInfo));
		outFile.write((char*)retTileDataInfos.data(), retTileDataInfos.size() * sizeof(Renderer::ReTextureFileFormat::TileDataInfo));
		outFile.write((char*)textureData.data(), (UINT)textureData.size());
	}

	void TextureProcessor::GenerateTextureAtlasFile2(const std::string& srcFilename, const std::string& dstFilename) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		std::string ext = Tool::StrUtil::GetFileExtension(srcFilename);

		Microsoft::WRL::ComPtr<IDStorageCompressionCodec> compressor;
		DSTORAGE_COMPRESSION compressionLevel = DSTORAGE_COMPRESSION_BEST_RATIO;
		uint32_t compressionFormat = 1u;
		HRASSERT(DStorageCreateCompressionCodec((DSTORAGE_COMPRESSION_FORMAT)compressionFormat, 2, IID_PPV_ARGS(&compressor)),
			"DStorageCreateCompressionCodec Failed");

		Renderer::ReTextureFileFormat::FileHeader retFileHeader;
		std::vector<Renderer::ReTextureFileFormat::SubresourceInfo> retSubresourceInfos;
		std::vector<Renderer::ReTextureFileFormat::TileDataInfo> retTileDataInfos;
		std::vector<uint8_t> textureData;

		DirectX::ScratchImage srcImage;
		if (ext == "dds") {
			HRASSERT(DirectX::LoadFromDDSFile(
				Tool::StrUtil::UTF8ToWString(srcFilename).c_str(),
				DirectX::DDS_FLAGS::DDS_FLAGS_NONE,
				nullptr,
				srcImage
			));
		}
		else if (ext == "png") {
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(srcFilename).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				srcImage
			));
		}

		const auto& metadata = srcImage.GetMetadata();
		uint32_t srcElementByteSize = srcImage.GetImages()[0].rowPitch / srcImage.GetImages()[0].width;

		retFileHeader.compressionFormat = compressionFormat;
		retFileHeader.dxgiFormat = metadata.format;
		retFileHeader.mipLevels = metadata.mipLevels;
		retFileHeader.arraySize = metadata.arraySize;
		retFileHeader.imageWidth = metadata.width;
		retFileHeader.imageHeight = metadata.height;

		// Create Reserved Resource
		Microsoft::WRL::ComPtr<ID3D12Device8> device;
		HRASSERT(D3D12CreateDevice(0, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

		auto d3dResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
		// Layout must be D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE when creating reserved resources
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRASSERT(device->CreateReservedResource(&d3dResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));

		UINT numTilesTotal = 0u;
		D3D12_PACKED_MIP_INFO packedMipInfo{};	// last n mips may be packed into a single tile
		D3D12_TILE_SHAPE tileShape{};			// e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		std::vector<D3D12_SUBRESOURCE_TILING> subresourceTiling;

		// query the reserved resource for its tile properties
		// allocate data structure according to tile properties
		UINT subresourceCount = metadata.arraySize * metadata.mipLevels;
		subresourceTiling.resize(subresourceCount);
		device->GetResourceTiling(resource.Get(),
			&numTilesTotal,
			&packedMipInfo,
			&tileShape, &subresourceCount, 0,
			subresourceTiling.data());

		retSubresourceInfos.resize(subresourceCount);
		UINT subresourceTileIndex = 0;
		for (UINT s = 0; s < subresourceCount; s++) {
			retSubresourceInfos[s].widthTiles = subresourceTiling[s].WidthInTiles;
			retSubresourceInfos[s].heightTiles = subresourceTiling[s].HeightInTiles;
			retSubresourceInfos[s].depthTiles = subresourceTiling[s].DepthInTiles;
			retSubresourceInfos[s].subresourceTileIndex = subresourceTileIndex;

			subresourceTileIndex += subresourceTiling[s].WidthInTiles * subresourceTiling[s].HeightInTiles;
		}
		retFileHeader.subresourceNums = subresourceCount;
		retFileHeader.tileNums = numTilesTotal;
		retFileHeader.tileWidth = tileShape.WidthInTexels;
		retFileHeader.tileHeight = tileShape.HeightInTexels;
		retFileHeader.tileRowPitch = srcElementByteSize * retFileHeader.tileWidth;
		retFileHeader.tileSlicePitch = retFileHeader.tileRowPitch * retFileHeader.tileHeight;

		auto writeTile = [&](uint8_t* pDst, const D3D12_TILED_RESOURCE_COORDINATE& in_coord, const DirectX::Image& srcImage) {
			// this is a BC7 or BC1 decoder
			// we know that tiles will be 64KB
			// 1 tile of BC7 size 256x256 will have a row size of 1024 bytes, and 64 rows (4 texels per row)
			// 1 tile of BC1 size 512x256 will also have row size 1024 bytes and 64 rows
			const UINT tileRowBytes = 1024;
			const UINT numRowsPerTile = 64;

			UINT srcOffset = 0u;

			// offset into this tile
			UINT startRow = in_coord.Y * numRowsPerTile;
			srcOffset += (srcImage.rowPitch * startRow);
			srcOffset += in_coord.X * tileRowBytes;

			// copy the rows of this tile
			for (UINT row = 0; row < numRowsPerTile; row++) {
				memcpy(pDst, srcImage.pixels + srcOffset, tileRowBytes);

				pDst += tileRowBytes;
				srcOffset += srcImage.rowPitch;
			}
		};

		auto compressTile = [&](std::vector<uint8_t>& tileData, size_t uncompressedDataSize) -> uint32_t {
			auto bound = compressor->CompressBufferBound(uncompressedDataSize);
			std::vector<uint8_t> scratch(bound);

			size_t compressedDataSize = 0;
			HRESULT hr = compressor->CompressBuffer(tileData.data(), uncompressedDataSize, compressionLevel, scratch.data(), bound, &compressedDataSize);
			ASSERT_FORMAT(compressedDataSize <= uncompressedDataSize);

			scratch.resize(compressedDataSize);
			tileData.swap(scratch);

			return compressedDataSize;
		};

		uint32_t offset = 0;
		std::vector<uint8_t> tile;
		for (uint32_t arrayIndex = 0; arrayIndex < metadata.arraySize; arrayIndex++) {
			for (uint32_t mipIndex = 0; mipIndex < metadata.mipLevels; mipIndex++) {
				uint32_t subresourceIndex = arrayIndex * metadata.mipLevels + mipIndex;
				auto& currImage = srcImage.GetImages()[subresourceIndex];
				const auto& currSubresourceInfo = retSubresourceInfos.at(subresourceIndex);

				for (uint32_t y = 0; y < currSubresourceInfo.heightTiles; y++) {
					for (uint32_t x = 0; x < currSubresourceInfo.widthTiles; x++) {
						tile.resize(D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES); // reset to standard tile size
						writeTile(tile.data(), D3D12_TILED_RESOURCE_COORDINATE{ x, y, 0, subresourceIndex }, currImage);

						if (retFileHeader.compressionFormat) {
							compressTile(tile, tile.size());
						}

						textureData.resize(textureData.size() + tile.size()); // grow the texture space to hold the new tile
						memcpy(&textureData[offset], tile.data(), tile.size()); // copy bytes

						// add tileData to array
						Renderer::ReTextureFileFormat::TileDataInfo outData{};
						outData.offset = offset;
						outData.numBytes = (UINT)tile.size();
						retTileDataInfos.push_back(outData);

						offset = (UINT)textureData.size();
					}
				}
			}
		}


		std::ofstream outFile(dstFilename, std::ios::out | std::ios::binary);

		UINT64 textureDataOffset = sizeof(retFileHeader) +
			(retSubresourceInfos.size() * sizeof(Renderer::ReTextureFileFormat::SubresourceInfo)) +
			(retTileDataInfos.size() * sizeof(Renderer::ReTextureFileFormat::TileDataInfo));

		// correct the tile offsets to account for the preceding data
		for (auto& o : retTileDataInfos) {
			o.offset += (UINT)textureDataOffset;
		}

		outFile.write((char*)&retFileHeader, sizeof(Renderer::ReTextureFileFormat::FileHeader));
		outFile.write((char*)retSubresourceInfos.data(), retSubresourceInfos.size() * sizeof(retSubresourceInfos[0]));
		outFile.write((char*)retTileDataInfos.data(), retTileDataInfos.size() * sizeof(retTileDataInfos[0]));
		outFile.write((char*)textureData.data(), (UINT)textureData.size());
	}

	void TextureProcessor::ConvertSplatMap(const std::string& dirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& targetFilename) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		// 读取全部的SplatMap
		std::vector<DirectX::ScratchImage> srcImages;
		srcImages.resize(endIndex - startIndex + 1);
		for (uint32_t i = 0; i <= endIndex; i++) {
			std::string srcFilename = dirname + baseFilename + std::to_string(i) + ".png";
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(srcFilename).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				srcImages[i]
			));
		}
		ASSERT_FORMAT(srcImages.size() > 0);

		size_t srcWidth = srcImages[0].GetImages()[0].width;
		size_t srcHeight = srcImages[0].GetImages()[0].height;
		size_t srcRowPitch = srcImages[0].GetImages()[0].rowPitch;
		DXGI_FORMAT srcFormat = srcImages[0].GetImages()[0].format;
		uint32_t srcElementByteSize = 4;	// RGBA
		// 合并后的SplatMap，是从所有混合纹理中挑选占比最高的四个纹理
		uint32_t dstElementByteSize = 8;
		DirectX::Image dstImage;
		dstImage.width = srcWidth;
		dstImage.height = srcHeight;
		dstImage.format = DXGI_FORMAT_R16G16B16A16_UINT;			// 每一个R16对应一个纹理，其中高8位为纹理索引，低8位为纹理权重
		dstImage.rowPitch = dstImage.width * dstElementByteSize;
		dstImage.slicePitch = dstImage.width * dstImage.height * dstElementByteSize;
		dstImage.pixels = new uint8_t[dstImage.slicePitch];

		struct TexturePair {
		public:
			uint8_t indexInTextureArray;
			uint8_t weight;

		public:
			TexturePair(uint8_t indexInTextureArray, uint8_t weight) : indexInTextureArray(indexInTextureArray), weight(weight) {}
			~TexturePair() {}

			const auto& GetU16Data() const { return (uint16_t)indexInTextureArray << 8 | weight; }
		};

		for (uint32_t i = 0; i < srcHeight; i++) {
			for (uint32_t j = 0; j < srcWidth; j++) {
				uint32_t srcOffset = i * srcRowPitch + j * srcElementByteSize;
				uint32_t dstOffset = i * dstImage.rowPitch + j * dstElementByteSize;

				std::vector<TexturePair> texturePairs;
				uint8_t textureIndexOffset = 0u;
				for (auto& srcImage : srcImages) {
					uint8_t* currPixel = srcImage.GetImages()[0].pixels + srcOffset;
					for (uint32_t k = 0; k < 4; k++) {
						uint8_t u8;
						memcpy(&u8, currPixel + k * sizeof(uint8_t), sizeof(uint8_t));

						texturePairs.emplace_back(textureIndexOffset, u8);

						textureIndexOffset += 1u;
					}
				}

				std::sort(texturePairs.begin(), texturePairs.end(), [](const auto& a, const auto& b) {
					return a.weight > b.weight;
				});

				{
					// 从TexturePair中挑选权重最高的四个纹理
					uint8_t* currPixel = dstImage.pixels + dstOffset;
					for (uint32_t k = 0; k < 4; k++) {
						const auto& currTexturePair = texturePairs.at(k);
						uint16_t u16Data = currTexturePair.GetU16Data();
						memcpy(currPixel + k * sizeof(uint16_t), &u16Data, sizeof(uint16_t));
					}
				}
			}
		}

		HRASSERT(DirectX::SaveToDDSFile(
			dstImage,
			DirectX::DDS_FLAGS_NONE,
			Tool::StrUtil::UTF8ToWString(targetFilename).c_str()
		));
	}

	void TextureProcessor::ResolveConvertedSplatMap(const std::string& srcFilename, const std::string& dstFilename) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		DirectX::ScratchImage srcImage;
		HRASSERT(DirectX::LoadFromDDSFile(
			Tool::StrUtil::UTF8ToWString(srcFilename).c_str(),
			DirectX::DDS_FLAGS_NONE,
			nullptr,
			srcImage
		));

		size_t srcWidth = srcImage.GetImages()[0].width;
		size_t srcHeight = srcImage.GetImages()[0].height;
		size_t srcRowPitch = srcImage.GetImages()[0].rowPitch;
		DXGI_FORMAT srcFormat = srcImage.GetImages()[0].format;
		uint32_t srcElementByteSize = 8;	// R16G16B16A16
		for (uint32_t i = 0; i < srcHeight; i++) {
			for (uint32_t j = 0; j < srcWidth; j++) {
				size_t srcOffset = i * srcRowPitch + j * srcElementByteSize;
				uint8_t* currPixel = srcImage.GetImages()[0].pixels + srcOffset;
				for (uint32_t k = 0; k < 4; k++) {
					uint16_t u16;
					memcpy(&u16, currPixel + k * sizeof(uint16_t), sizeof(uint16_t));
					uint8_t textureIndex = (uint8_t)(u16 >> 8);
					uint8_t weight = (uint8_t)(u16 & 0x00ff);
					int32_t i = 0;
				}
			}
		}
	}

	void TextureProcessor::GenerateTextureArray(const std::string& srcDirname, const std::string& dstFilename) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		// 读取原始图像
		std::vector<DirectX::ScratchImage> srcImages;
		std::filesystem::path srcPath = srcDirname;
		for (const auto& entry : std::filesystem::directory_iterator(srcPath)) {
			if (!entry.is_regular_file()) {
				continue;
			}

			DirectX::ScratchImage srcImage;
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(entry.path().string()).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				srcImage
			));

			srcImages.emplace_back(std::move(srcImage));
		}
		ASSERT_FORMAT(srcImages.size() > 0);
		
		// 为原始图像生成带MipChain的图像
		// 计算生成的最大级别(规定MipChain上最大级别的大小不小于64KB)
		uint32_t srcWidth = srcImages[0].GetImages()[0].width;
		uint32_t elementByteSize = srcImages[0].GetImages()[0].rowPitch / srcWidth;
		uint32_t minAllowedWidth = std::sqrt(65536 / elementByteSize);
		uint32_t maxAllowedLevels = std::log2(srcWidth / minAllowedWidth) + 1;

		std::vector<DirectX::ScratchImage> mipImages;
		mipImages.resize(srcImages.size());
		for (uint32_t i = 0; i < mipImages.size(); i ++) {
			auto& srcImage = srcImages[i];
			auto& mipImage = mipImages[i];

			HRASSERT(DirectX::GenerateMipMaps(
				srcImage.GetImages()[0], DirectX::TEX_FILTER_DEFAULT, maxAllowedLevels, mipImage
			));
		}

		std::vector<DirectX::Image> dstImages;
		for (uint32_t arrayIndex = 0; arrayIndex < mipImages.size(); arrayIndex++) {
			auto& currMipImage = mipImages[arrayIndex];
			for (uint32_t mipIndex = 0; mipIndex < currMipImage.GetImageCount(); mipIndex++) {
				auto& currMip = currMipImage.GetImages()[mipIndex];
				dstImages.push_back(currMip);
			}
		}

		DirectX::TexMetadata metaData{};
		metaData.width = mipImages[0].GetImages()[0].width;
		metaData.height = mipImages[0].GetImages()[0].height;
		metaData.format = mipImages[0].GetImages()[0].format;
		metaData.arraySize = mipImages.size();
		metaData.depth = 1u;
		metaData.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
		metaData.mipLevels = maxAllowedLevels;

		HRASSERT(DirectX::SaveToDDSFile(
			dstImages.data(), dstImages.size(), metaData,
			DirectX::DDS_FLAGS_NONE,
			Tool::StrUtil::UTF8ToWString(dstFilename).c_str()
		));
	}

	void TextureProcessor::GenerateRandomAlbedoMap(const std::string& dstFilename, uint32_t width, uint32_t height) {
		// 定义一个颜色结构体，用于存储 RGB 值
		struct Color {
			int r;
			int g;
			int b;
			Color() = default;
			Color(int red, int green, int blue) : r(red), g(green), b(blue) {}
		};

		// 生成随机不同的颜色
		auto generateUniqueColors = [](int numColors) -> std::vector<Color> {
			std::vector<Color> colors;
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dis(0, 255);

			for (int i = 0; i < numColors; ++i) {
				int r = dis(gen);
				int g = dis(gen);
				int b = dis(gen);
				Color newColor(r, g, b);
				colors.push_back(newColor);
			}

			// 移除相似的颜色，确保生成的颜色是不同的
			auto it = std::unique(colors.begin(), colors.end(), [](const Color& c1, const Color& c2) {
				return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
				});
			colors.resize(std::distance(colors.begin(), it));

			return colors;
		};

		// 创建一个指定大小和格式的图像
		cv::Mat texture(height, width, CV_8UC4);

		// 生成64种不同颜色
		std::vector<Color> colors = generateUniqueColors(64);

		// 用64种颜色填充纹理图
		int colorIndex = 0;
		int blockSize = width / 8; // 每个颜色块的大小

		for (int y = 0; y < height; y += blockSize) {
			for (int x = 0; x < width; x += blockSize) {
				cv::Rect block_rect(x, y, blockSize, blockSize);
				cv::Scalar block_color(colors[colorIndex].b, colors[colorIndex].g, colors[colorIndex].r, 255); // A 通道值为 1（255）
				cv::rectangle(texture, block_rect, block_color, -1); // 使用矩形填充颜色
				colorIndex = (colorIndex + 1) % 64;
			}
		}

		cv::imwrite(dstFilename, texture);

	}

	void TextureProcessor::GenerateTerrainQuadNodeDescriptors(const std::string& heightMapFilename, const std::string& dstDirname) {
		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
			coInitialized = true;
		}

		/*
		* 节点描述
		*/
		struct NodeDescriptor {
		public:
			float minHeight;		// HeightMap为R16格式
			float maxHeight;		// HeightMap为R16格式

			uint32_t tilePosX{ 255u };
			uint32_t tilePosY{ 255u };
		};

		/*
		* Lod描述
		*/
		struct LodDescriptor {
		public:
			uint32_t nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
			uint32_t nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
			uint32_t nodeCount;			// 该LOD中的Node的总个数

			float pad1;
		};

		uint32_t maxLOD{ 4u };	// 最大LOD等级
		uint32_t mostDetailNodeMeterSize{ 64u }; // 最精细的节点的大小(单位: 米)
		uint32_t nodeCount{ 0u };
		float worldMeterSize{ 8192.0f };

		// 创建NodeDescriptorArray
		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LodDescriptor> lodDescriptors;

		lodDescriptors.resize(maxLOD + 1u);
		for (int32_t i = maxLOD; i >= 0; i--) {
			auto& lodDescriptor = lodDescriptors.at(i);

			uint32_t currDetailNodeSize = mostDetailNodeMeterSize * pow(2, i);
			uint32_t nodeCountPerAxis = worldMeterSize / currDetailNodeSize;

			lodDescriptor.nodeMeterSize = currDetailNodeSize;
			lodDescriptor.nodeStartOffset = nodeCount;
			lodDescriptor.nodeCount = nodeCountPerAxis * nodeCountPerAxis;

			nodeCount += lodDescriptor.nodeCount;
		}
		nodeDescriptors.resize(nodeCount);

		// 读取HeightMap
		DirectX::ScratchImage srcImage;
		HRASSERT(DirectX::LoadFromWICFile(
			Tool::StrUtil::UTF8ToWString(heightMapFilename).c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			srcImage
		));

		uint32_t srcWidth = srcImage.GetImages()[0].width;
		uint32_t srcHeight = srcImage.GetImages()[0].height;
		uint32_t elementByteSize = srcImage.GetImages()[0].rowPitch / srcImage.GetImages()[0].width;
		ASSERT_FORMAT(srcWidth == srcHeight, "Muse Equal");

		for (int32_t lodIndex = maxLOD; lodIndex >= 0; lodIndex--) {
			const auto& lodDescriptor = lodDescriptors.at(lodIndex);
			uint32_t currLODNodeMeterSize = pow(2, lodIndex) * mostDetailNodeMeterSize;
			uint32_t subSize = currLODNodeMeterSize + 1;
			uint32_t step = currLODNodeMeterSize;

			DirectX::Image dstImage;
			dstImage.width = subSize;
			dstImage.height = subSize;
			dstImage.format = srcImage.GetImages()[0].format;
			dstImage.rowPitch = dstImage.width * elementByteSize;
			dstImage.slicePitch = dstImage.width * dstImage.height * elementByteSize;
			dstImage.pixels = new uint8_t[dstImage.slicePitch];

			uint32_t localOffset = 0u;
			for (uint32_t j = 0; j < srcHeight - 1; j += step) {
				for (uint32_t i = 0; i < srcWidth - 1; i += step) {
					// 当前评估的节点
					uint32_t globalOffset = lodDescriptor.nodeStartOffset + localOffset;
					localOffset++;

					// 计算切分后图像相对于原始图形的起始偏移量
					uint64_t srcOffset = j * srcImage.GetImages()[0].rowPitch + i * elementByteSize;
					uint64_t dstOffset = 0u;

					for (uint32_t index = 0; index < dstImage.height; index++) {
						memcpy(dstImage.pixels + dstOffset, srcImage.GetImages()[0].pixels + srcOffset, dstImage.rowPitch);

						srcOffset += srcImage.GetImages()[0].rowPitch;
						dstOffset += dstImage.rowPitch;
					}

					uint32_t binOffset = 0u;
					uint16_t minHeight = 65535u;
					uint16_t maxHeight = 0u;
					for (uint32_t index = 0; index < dstImage.height; index++) {
						for (uint32_t groupIndex = 0; groupIndex < dstImage.width; groupIndex++) {
							// 读取一个组下的RGBA值
							uint16_t u16R = 0;
							// uint16_t u16G = 0;
							// uint16_t u16B = 0;
							// uint16_t u16A = 0;
							memcpy(&u16R, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
							// memcpy(&u16G, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
							// memcpy(&u16B, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
							// memcpy(&u16A, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;

							// assert((u16R == u16G) && (u16G == u16B) && u16A == 65535);
							minHeight = u16R < minHeight ? u16R : minHeight;
							maxHeight = u16R > maxHeight ? u16R : maxHeight;
						}
					}

					auto& currNodeDescriptor = nodeDescriptors.at(globalOffset);
					currNodeDescriptor.minHeight = (float)minHeight / (float)65535.0f;
					currNodeDescriptor.maxHeight = (float)maxHeight / (float)65535.0f;
				}
			}
		}

		std::ofstream terrainNodeDescriptorStream(dstDirname + "TerrainNodeDescriptor.bin", std::ios::binary | std::ios::out);
		size_t terrainNodeDescriptorSize = nodeDescriptors.size();
		terrainNodeDescriptorStream.write((char*)&terrainNodeDescriptorSize, sizeof(size_t));
		terrainNodeDescriptorStream.write((char*)nodeDescriptors.data(), nodeDescriptors.size() * sizeof(NodeDescriptor));
		terrainNodeDescriptorStream.close();

		std::ofstream terrainLodDescriptorStream(dstDirname + "TerrainLodDescriptor.bin", std::ios::binary | std::ios::out);
		size_t terrainLodDescriptorSize = lodDescriptors.size();
		terrainLodDescriptorStream.write((char*)&terrainLodDescriptorSize, sizeof(size_t));
		terrainLodDescriptorStream.write((char*)lodDescriptors.data(), lodDescriptors.size() * sizeof(LodDescriptor));
		terrainLodDescriptorStream.close();
	}

}