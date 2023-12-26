#include "OfflineTask/TextureProcessor.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"
#include "Math/Helper.h"

#include <wincodec.h>
#include <fstream>
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

		static uint32_t uid = startNameIndex;
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

		// 转换为 OpenCV 的 Mat 格式
		cv::Mat originalImage(srcHeight, srcWidth, CV_16UC1, pixel);

		// 定义目标图像尺寸
		cv::Size targetSize(targetWidth, targetHeight);

		// 缩放图像
		cv::Mat resizedImage;
		cv::resize(originalImage, resizedImage, targetSize, 0, 0, cv::INTER_CUBIC);
		
		// 可选：保存缩放后的图像
		cv::imwrite(dirname + Tool::StrUtil::GetFilename(filename) + ".png", resizedImage);
	}

	void TextureProcessor::MergeTextureAtlasFile(const std::string& dirname, const std::string& baseFilename, uint32_t startIndex, uint32_t endIndex, const std::string& targetFilename) {
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

		struct FileHeader {
			UINT32 compressionFormat{ 1u };	// 0 is no compression
			DXGI_FORMAT dxgiFormat;
			uint32_t tileWidth;
			uint32_t tileHeight;
			uint32_t tileRowPitch;			// 注意！纹理的每一行（除了最后一行）都需要256字节对齐，并且纹理本身也要256字节对齐
			uint32_t tileSlicePitch;
			uint32_t tileCount;				// 描述总共有多少Tile
		};
		struct TileDataDescriptor {
			UINT32 offset;					// file offset to tile data
			UINT32 numBytes;				// # bytes for the tile
		};

		FileHeader fileHeader;
		std::vector<TileDataDescriptor> tileDataDescriptors;	// offsets table
		std::vector<uint8_t> textureData;

		auto compressTileData = [&](std::vector<uint8_t>& tileData, size_t uncompressedDataSize) -> uint32_t{
			auto bound = compressor->CompressBufferBound(uncompressedDataSize);
			std::vector<uint8_t> scratch(bound);

			size_t compressedDataSize = 0;
			HRESULT hr = compressor->CompressBuffer(tileData.data(), uncompressedDataSize, compressionLevel, scratch.data(), bound, &compressedDataSize);
			ASSERT_FORMAT(compressedDataSize <= uncompressedDataSize);

			scratch.resize(compressedDataSize);
			tileData.swap(scratch);

			return compressedDataSize;
		};

		for (uint32_t i = 0; i <= endIndex; i++) {
			std::string srcFilename = dirname + baseFilename + std::to_string(i) + ".png";

			DirectX::ScratchImage srcImage;
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(srcFilename).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				srcImage
			));

			// 注意，CPU端的图像的RowPitch、SlicePitch与GPU端的存储是不同的，因此需要特别注意
			uint32_t srcWidth = srcImage.GetImages()[0].width;
			uint32_t srcHeight = srcImage.GetImages()[0].height;
			uint32_t srcRowPitch = srcImage.GetImages()[0].rowPitch;
			uint32_t srcSlicePitch = srcImage.GetImages()[0].slicePitch;
			DXGI_FORMAT srcDxgiFormat = srcImage.GetImages()[0].format;
			uint8_t* srcData = srcImage.GetImages()[0].pixels;

			uint32_t tileRowPitch = Math::AlignUp(srcRowPitch, 256);
			uint32_t tileSlicePitch = Math::AlignUp(tileRowPitch * (srcHeight - 1) + srcRowPitch, 256);
			std::vector<uint8_t> tileData(tileSlicePitch);

			if (i != 0) {
				ASSERT_FORMAT(fileHeader.dxgiFormat == srcDxgiFormat);
				ASSERT_FORMAT(fileHeader.tileWidth == srcWidth);
				ASSERT_FORMAT(fileHeader.tileHeight == srcHeight);
				ASSERT_FORMAT(fileHeader.tileRowPitch == tileRowPitch);
				ASSERT_FORMAT(fileHeader.tileSlicePitch == tileSlicePitch);
			}
			else {
				fileHeader.dxgiFormat = srcDxgiFormat;
				fileHeader.tileWidth = srcWidth;
				fileHeader.tileHeight = srcHeight;
				fileHeader.tileRowPitch = tileRowPitch;
				fileHeader.tileSlicePitch = tileSlicePitch;
				fileHeader.tileCount = endIndex - startIndex + 1;
			}

			for (uint32_t row = 0; row < srcHeight; row++) {
				uint32_t srcOffset = row * srcRowPitch;
				uint32_t dstOffset = row * tileRowPitch;
				memcpy(tileData.data() + dstOffset, srcData + srcOffset, srcRowPitch);
			}


			uint32_t textureDataOffset = textureData.size();
			uint32_t compressedTileDataSize = compressTileData(tileData, tileSlicePitch);

			TileDataDescriptor tileDataDescriptor;
			tileDataDescriptor.offset = textureDataOffset;
			tileDataDescriptor.numBytes = compressedTileDataSize;
			tileDataDescriptors.emplace_back(std::move(tileDataDescriptor));

			textureData.resize(textureData.size() + compressedTileDataSize);
			memcpy(textureData.data() + textureDataOffset, tileData.data(), compressedTileDataSize);
		}

		uint64_t textureDataOffsetInFile = sizeof(FileHeader) + sizeof(TileDataDescriptor) * tileDataDescriptors.size();
		for (auto& descriptor : tileDataDescriptors) {
			descriptor.offset += textureDataOffsetInFile;
		}

		std::ofstream outFile(targetFilename, std::ios::out | std::ios::binary);

		outFile.write((char*)&fileHeader, sizeof(FileHeader));
		outFile.write((char*)tileDataDescriptors.data(), tileDataDescriptors.size() * sizeof(TileDataDescriptor));
		outFile.write((char*)textureData.data(), (UINT)textureData.size());
	}

}