#include "Renderer/FixedTextureHelper.h"
#include "Renderer/FormatConverter.h"

#include "GHL/Fence.h"
#include "GHL/Box.h"

#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

#include "DirectXTex/DirectXTex.h"

namespace Renderer {

	TextureWrap FixedTextureHelper::LoadFromFile(
		const GHL::Device* device,
		PoolDescriptorAllocator* descriptorAllocator,
		ResourceAllocator* resourceAllocator,
		IDStorageQueue* copyQueue,
		GHL::Fence* copyFence,
		const std::string& filepath) {
		std::string name = Tool::StrUtil::RemoveBasePath(filepath);
		std::string ext  = Tool::StrUtil::GetFileExtension(filepath);

		// 读取纹理至内存
		DirectX::ScratchImage baseImage;
		if (ext == "dds") {
			HRASSERT(DirectX::LoadFromDDSFile(
				Tool::StrUtil::UTF8ToWString(filepath).c_str(),
				DirectX::DDS_FLAGS_NONE,
				nullptr,
				baseImage
			));
		}
		else if (ext == "png") {
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(filepath).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_IGNORE_SRGB,
				nullptr,
				baseImage
			));
		}
		else if (ext == "xet") {

		}
		else {
			ASSERT_FORMAT(false, "Unsupported Texture Type");
		}

		// 创建GPU对象
		Renderer::TextureDesc textureDesc = FormatConverter::GetTextureDesc(baseImage.GetMetadata());
		textureDesc.expectedState |= GHL::EResourceState::AnyShaderAccess;

		TextureWrap textureWrap = resourceAllocator->Allocate(
			device, textureDesc, descriptorAllocator, nullptr);

		// 获取GPU对象存储信息
		uint32_t subresourceCount = textureWrap->GetResourceFormat().SubresourceCount();
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
		std::vector<uint32_t> numRows(subresourceCount);
		std::vector<uint64_t> rowSizeInBytes(subresourceCount);
		uint64_t requiredSize = 0u;
		auto d3dResDesc = textureWrap->GetResourceFormat().D3DResourceDesc();
		device->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
			placedLayouts.data(), numRows.data(), rowSizeInBytes.data(), &requiredSize);

		// 纹理行数据对齐
		uint8_t* alignedData = new uint8_t[requiredSize];
		uint32_t subresourceIndex = 0;
		for (uint32_t arrayIndex = 0; arrayIndex < textureDesc.arraySize; arrayIndex++) {
			for (uint32_t mipIndex = 0; mipIndex < textureDesc.mipLevals; mipIndex++) {
				auto* image = &baseImage.GetImages()[subresourceIndex];
				for (uint32_t rowIndex = 0u; rowIndex < numRows[subresourceIndex]; rowIndex++) {
					uint32_t realByteOffset = rowIndex * placedLayouts.at(subresourceIndex).Footprint.RowPitch + placedLayouts.at(subresourceIndex).Offset;
					uint32_t fakeByteOffset = rowIndex * image->rowPitch;

					memcpy(alignedData + realByteOffset, image->pixels + fakeByteOffset, image->rowPitch);
				}

				subresourceIndex++;
			}
		}

		/*
		for (uint32_t subresourceIndex = 0u; subresourceIndex < subresourceCount; subresourceIndex++) {
			for (uint32_t sliceIndex = 0u; sliceIndex < placedLayouts.at(subresourceIndex).Footprint.Depth; sliceIndex++) {
				auto* image = baseImage.GetImage(subresourceIndex, 0u, sliceIndex);

				for (uint32_t rowIndex = 0u; rowIndex < numRows[subresourceIndex]; rowIndex++) {
					uint32_t realByteOffset =
						rowIndex * placedLayouts.at(subresourceIndex).Footprint.RowPitch +
						sliceIndex * numRows[subresourceIndex] * placedLayouts.at(subresourceIndex).Footprint.RowPitch +
						placedLayouts.at(subresourceIndex).Offset;
					uint32_t fakeByteOffset = rowIndex * image->rowPitch;
					memcpy(alignedData + realByteOffset, image->pixels + fakeByteOffset, image->rowPitch);
				}
			}
		}
		*/

		// 上传至显存
		DSTORAGE_REQUEST request = {};
		request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
		request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
		request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
		request.Source.Memory.Source = alignedData;
		request.Source.Memory.Size = requiredSize;
		request.Destination.MultipleSubresources.FirstSubresource = 0u;
		request.Destination.MultipleSubresources.Resource = textureWrap->D3DResource();
		request.UncompressedSize = requiredSize;

		copyQueue->EnqueueRequest(&request);
		copyFence->IncrementExpectedValue();
		copyQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
		copyQueue->Submit();
		copyFence->Wait();

		delete alignedData;
		baseImage.Release();

		textureWrap->SetDebugName(name);
		return textureWrap;
	}

	void        FixedTextureHelper::SaveToFile(
		const GHL::Device* device,
		TextureWrap& textureWrap,	// GPU纹理对象 
		BufferWrap& rbBufferWrap,	// GPU回读数据
		const std::string& filepath) {
		std::string name = Tool::StrUtil::RemoveBasePath(filepath);
		std::string ext = Tool::StrUtil::GetFileExtension(filepath);

		uint32_t subresourceCount = textureWrap->GetResourceFormat().SubresourceCount();
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
		std::vector<uint32_t> numRows(subresourceCount);
		std::vector<uint64_t> rowSizeInBytes(subresourceCount);
		uint64_t requiredSize = 0u;
		auto d3dResDesc = textureWrap->GetResourceFormat().D3DResourceDesc();
		device->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
			placedLayouts.data(), numRows.data(), rowSizeInBytes.data(), &requiredSize);

		// 填充CPU纹理对象并进行存储
		std::vector<DirectX::Image> images;
		for (uint32_t subresourceIndex = 0u; subresourceIndex < subresourceCount; subresourceIndex++) {
			auto& currPlacedLayouts = placedLayouts.at(subresourceIndex);
			auto& currNumRows = numRows.at(subresourceIndex);
			auto& currRowSizeInBytes = rowSizeInBytes.at(subresourceIndex);

			for (uint32_t sliceIndex = 0u; sliceIndex < currPlacedLayouts.Footprint.Depth; sliceIndex++) {
				auto slicePitch = currPlacedLayouts.Footprint.RowPitch * currNumRows;
				auto currOffset = currPlacedLayouts.Offset + (sliceIndex * slicePitch);

				DirectX::Image currImage;
				currImage.width = currPlacedLayouts.Footprint.Width;
				currImage.height = currPlacedLayouts.Footprint.Height;
				currImage.format = currPlacedLayouts.Footprint.Format;
				currImage.rowPitch = currPlacedLayouts.Footprint.RowPitch;
				currImage.slicePitch = slicePitch;
				currImage.pixels = new uint8_t[currImage.slicePitch];

				memcpy(currImage.pixels, rbBufferWrap->Map() + currOffset, slicePitch);

				images.emplace_back(std::move(currImage));
			}
		}

		DirectX::TexMetadata metadata = FormatConverter::GetTexMetadata(textureWrap->GetResourceFormat().GetTextureDesc());
		
		if (ext == "png") {
			DirectX::SaveToWICFile(images.data(), images.size(), WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), Tool::StrUtil::UTF8ToWString(filepath).c_str());
		}
		else if (ext == "dds") {
			DirectX::SaveToDDSFile(images.data(), images.size(), metadata, DirectX::DDS_FLAGS_NONE, Tool::StrUtil::UTF8ToWString(filepath).c_str());
		}
		else {
			ASSERT_FORMAT(false, "Unsupported Texture Type");
		}
	}

}