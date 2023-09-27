#include "Renderer/XetTextureGenerator.h"
#include "GHL/DebugLayer.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"
#include <filesystem>
#include <fstream>

namespace Renderer {

	void XetTextureGenerator::Generate(const std::string& inFilePathname, const std::string& outputFilePathname) {

        std::wstring wInFilePath = Tool::StrUtil::UTF8ToWString(inFilePathname);
        std::wstring wOutputFilePath = Tool::StrUtil::UTF8ToWString(outputFilePathname);
        //--------------------------
        // read dds file
        //--------------------------
        HANDLE inFileHandle = CreateFile(wInFilePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        ASSERT_FORMAT(inFileHandle != NULL, "Failed to open file");

        HANDLE inFileMapping = CreateFileMapping(inFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        ASSERT_FORMAT(inFileMapping != NULL, "Failed to create mapping");

        BYTE* pInFileBytes = (BYTE*)MapViewOfFile(inFileMapping, FILE_MAP_READ, 0, 0, 0);
        ASSERT_FORMAT(pInFileBytes != NULL, "Failed to map file");

        mXetHeader.m_compressionFormat = mCompressionFormat;

        //--------------------------
        // interpret contents based on dds header
        //--------------------------
        BYTE* pBits = pInFileBytes;
        if (DirectX::DDS_MAGIC == *(UINT32*)pBits) {
            pBits += sizeof(UINT32);
            mXetHeader.m_ddsHeader = *(DirectX::DDS_HEADER*)pBits;
            pBits += mXetHeader.m_ddsHeader.size;

            if ((mXetHeader.m_ddsHeader.ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == mXetHeader.m_ddsHeader.ddspf.fourCC)) {
                mXetHeader.m_extensionHeader = *(DirectX::DDS_HEADER_DXT10*)pBits;
                pBits += sizeof(DirectX::DDS_HEADER_DXT10);
            }
            else {
                DirectX::DDS_HEADER_DXT10 extensionHeader{};
                mXetHeader.m_extensionHeader = extensionHeader;
                mXetHeader.m_extensionHeader.dxgiFormat = GetFormatFromHeader(mXetHeader.m_ddsHeader);
            }
        }
        else {

        }

        GetTiling();
        FillSubresourceData();

        //--------------------------
        // reserve output space
        //--------------------------
        std::filesystem::path inFilePath(wInFilePath);
        auto fileSize = std::filesystem::file_size(inFilePath);

        mTextureData.reserve(fileSize); // reserve enough space to hold the whole uncompressed source
        mOffsets.reserve(mXetHeader.m_mipInfo.m_numTilesForStandardMips + 1);

        //--------------------------
        // write tiles
        //--------------------------
        if (mCompressionFormat) {
            HRESULT hr = DStorageCreateCompressionCodec((DSTORAGE_COMPRESSION_FORMAT)mCompressionFormat, 2, IID_PPV_ARGS(&mCompressor));
        }
        WriteTiles(pBits);
        mXetHeader.m_mipInfo.m_numUncompressedBytesForPackedMips = WritePackedMips(pInFileBytes, fileSize);

        //------------------------------------------
        // correct offsets to account for alignment after header
        //------------------------------------------
        UINT64 textureDataOffset = sizeof(mXetHeader) +
            (mXetSubresourceInfos.size() * sizeof(mXetSubresourceInfos[0])) +
            (mOffsets.size() * sizeof(mOffsets[0]));

        // align only for legacy support for uncompressed file formats
        std::vector<BYTE> alignedTextureDataGap;
        if (!mCompressionFormat) {
            UINT alignedTextureDataOffset = GetAlignedSize((UINT)textureDataOffset);
            alignedTextureDataGap.resize(alignedTextureDataOffset - textureDataOffset, 0);
            textureDataOffset += alignedTextureDataGap.size();
        }

        // correct the tile offsets to account for the preceding data
        for (auto& o : mOffsets) {
            o.m_offset += (UINT)textureDataOffset;
        }

        std::ofstream outFile(wOutputFilePath, std::ios::out | std::ios::binary);

        outFile.write((char*)&mXetHeader, sizeof(mXetHeader));
        outFile.write((char*)mXetSubresourceInfos.data(), mXetSubresourceInfos.size() * sizeof(mXetSubresourceInfos[0]));
        outFile.write((char*)mOffsets.data(), mOffsets.size() * sizeof(mOffsets[0]));

        // alignment is here only for legacy support for uncompressed file formats
        if (alignedTextureDataGap.size()) {
            outFile.write((char*)alignedTextureDataGap.data(), alignedTextureDataGap.size());
        }

        outFile.write((char*)mTextureData.data(), (UINT)mTextureData.size());
        outFile.write((char*)mPackedMipData.data(), (UINT)mPackedMipData.size());

        UnmapViewOfFile(pInFileBytes);
        CloseHandle(inFileMapping);
        CloseHandle(inFileHandle);
	}

    DXGI_FORMAT XetTextureGenerator::GetFormatFromHeader(const DirectX::DDS_HEADER& in_ddsHeader) {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

        if (in_ddsHeader.ddspf.flags & DDS_FOURCC) {
            UINT32 fourCC = in_ddsHeader.ddspf.fourCC;
            if (DirectX::DDSPF_DXT1.fourCC == fourCC) { format = DXGI_FORMAT_BC1_UNORM; }
            if (DirectX::DDSPF_DXT2.fourCC == fourCC) { format = DXGI_FORMAT_BC2_UNORM; }
            if (DirectX::DDSPF_DXT3.fourCC == fourCC) { format = DXGI_FORMAT_BC2_UNORM; }
            if (DirectX::DDSPF_DXT4.fourCC == fourCC) { format = DXGI_FORMAT_BC3_UNORM; }
            if (DirectX::DDSPF_DXT5.fourCC == fourCC) { format = DXGI_FORMAT_BC3_UNORM; }
            if (MAKEFOURCC('A', 'T', 'I', '1') == fourCC) { format = DXGI_FORMAT_BC4_UNORM; }
            if (MAKEFOURCC('A', 'T', 'I', '2') == fourCC) { format = DXGI_FORMAT_BC5_UNORM; }
            if (MAKEFOURCC('B', 'C', '4', 'U') == fourCC) { format = DXGI_FORMAT_BC4_UNORM; }
            if (MAKEFOURCC('B', 'C', '4', 'S') == fourCC) { format = DXGI_FORMAT_BC4_SNORM; }
            if (MAKEFOURCC('B', 'C', '5', 'U') == fourCC) { format = DXGI_FORMAT_BC5_UNORM; }
            if (MAKEFOURCC('B', 'C', '5', 'S') == fourCC) { format = DXGI_FORMAT_BC5_SNORM; }
        }
        if (DXGI_FORMAT_UNKNOWN == format) { ASSERT_FORMAT(false, "Texture Format Unknown"); }

        return format;
    }

    void XetTextureGenerator::GetTiling() {
        UINT numTilesTotal = 0u;
        D3D12_PACKED_MIP_INFO packedMipInfo{}; // last n mips may be packed into a single tile
        D3D12_TILE_SHAPE tileShape{}; // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
        std::vector<D3D12_SUBRESOURCE_TILING> subresourceTiling;

        UINT imageWidth = mXetHeader.m_ddsHeader.width;
        UINT imageHeight = mXetHeader.m_ddsHeader.height;
        UINT mipCount = mXetHeader.m_ddsHeader.mipMapCount == 0 ? 1 : mXetHeader.m_ddsHeader.mipMapCount;

        GHL::EnableDebugLayer();

        // Create Reserved Resource
        Microsoft::WRL::ComPtr<ID3D12Device8> device;
        HRASSERT(D3D12CreateDevice(0, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

        mD3D12ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mXetHeader.m_extensionHeader.dxgiFormat, imageWidth, imageHeight, 1u, mipCount);

        // Layout must be D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE when creating reserved resources
        mD3D12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        HRASSERT(device->CreateReservedResource(&mD3D12ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));

        // query the reserved resource for its tile properties
        // allocate data structure according to tile properties
        UINT subresourceCount = mipCount;
        subresourceTiling.resize(subresourceCount);
        device->GetResourceTiling(resource.Get(),
            &numTilesTotal,
            &packedMipInfo,
            &tileShape, &subresourceCount, 0,
            subresourceTiling.data());

        // pre-fill header information based on tiling
        mXetSubresourceInfos.resize(subresourceCount);
        UINT subresourceTileIndex = 0;
        for (UINT s = 0; s < packedMipInfo.NumStandardMips; s++) {
            mXetSubresourceInfos[s].m_standardMipInfo = XetFileHeader::StandardMipInfo{
                subresourceTiling[s].WidthInTiles,
                subresourceTiling[s].HeightInTiles,
                0u,
                subresourceTileIndex };

            // why not use StartTileIndexInOverallResource ?
            subresourceTileIndex += subresourceTiling[s].WidthInTiles * subresourceTiling[s].HeightInTiles;
        }

        mXetHeader.m_mipInfo.m_numStandardMips = packedMipInfo.NumStandardMips;
        mXetHeader.m_mipInfo.m_numTilesForStandardMips = numTilesTotal - packedMipInfo.NumTilesForPackedMips;
        mXetHeader.m_mipInfo.m_numPackedMips = packedMipInfo.NumPackedMips;
        mXetHeader.m_mipInfo.m_numTilesForPackedMips = packedMipInfo.NumTilesForPackedMips;
        mXetHeader.m_mipInfo.m_numUncompressedBytesForPackedMips = 0; // will be filled in later
    }

    void XetTextureGenerator::FillSubresourceData() {
        UINT offset = 0;

        UINT w = mXetHeader.m_ddsHeader.width;
        UINT h = mXetHeader.m_ddsHeader.height;

        UINT bytesPerElement = 0;

        switch (mXetHeader.m_extensionHeader.dxgiFormat) {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            bytesPerElement = 8;
            break;
        default: // BC7
            bytesPerElement = 16;
        }
        UINT numMips = mXetHeader.m_mipInfo.m_numStandardMips + mXetHeader.m_mipInfo.m_numPackedMips;
        mSourceSubresourceDatas.resize(numMips);
        for (size_t i = 0; i < numMips; i++) {
            UINT numBlocksWide = std::max<UINT>(1, (w + 3) / 4);
            UINT numBlocksHigh = std::max<UINT>(1, (h + 3) / 4);

            UINT rowBytes = numBlocksWide * bytesPerElement;
            UINT subresourceBytes = rowBytes * numBlocksHigh;

            mSourceSubresourceDatas[i] = SourceSubResourceData{ offset, rowBytes, subresourceBytes };

            offset += subresourceBytes;

            if (w > 1) { w >>= 1; }
            if (h > 1) { h >>= 1; }
        }
    }

    void XetTextureGenerator::WriteTiles(const BYTE* in_pSrc) {
        UINT imageWidth = mXetHeader.m_ddsHeader.width;
        UINT imageHeight = mXetHeader.m_ddsHeader.height;
        UINT mipCount = mXetHeader.m_ddsHeader.mipMapCount;

        // texture data starts after the header, and after the table of offsets
        UINT offset = 0;

        std::vector<BYTE> tile; // scratch space for writing tiled texture data

        // find the base address of each /tiled/ mip level
        for (UINT s = 0; s < mXetHeader.m_mipInfo.m_numStandardMips; s++) {
            for (UINT y = 0; y < mXetSubresourceInfos[s].m_standardMipInfo.m_heightTiles; y++) {
                for (UINT x = 0; x < mXetSubresourceInfos[s].m_standardMipInfo.m_widthTiles; x++) {
                    tile.resize(D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES); // reset to standard tile size

                    if (false) {
                        memcpy(tile.data(), in_pSrc, D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
                        in_pSrc += D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
                    }
                    else {
                        WriteTile(tile.data(), D3D12_TILED_RESOURCE_COORDINATE{ x, y, 0, s }, mSourceSubresourceDatas[s], in_pSrc);
                    }

                    if (mCompressionFormat) {
                        CompressTile(tile);
                    }

                    mTextureData.resize(mTextureData.size() + tile.size()); // grow the texture space to hold the new tile
                    memcpy(&mTextureData[offset], tile.data(), tile.size()); // copy bytes

                    // add tileData to array
                    XetFileHeader::TileData outData{ 0 };
                    outData.m_offset = offset;
                    outData.m_numBytes = (UINT)tile.size();
                    mOffsets.push_back(outData);

                    offset = (UINT)mTextureData.size();
                }
            }
        }
    }

    UINT XetTextureGenerator::WriteTile(BYTE* out_pDst,
        const D3D12_TILED_RESOURCE_COORDINATE& in_coord,
        const SourceSubResourceData& in_subresourceData,
        const BYTE* in_pSrc) {

        // this is a BC7 or BC1 decoder
        // we know that tiles will be 64KB
        // 1 tile of BC7 size 256x256 will have a row size of 1024 bytes, and 64 rows (4 texels per row)
        // 1 tile of BC1 size 512x256 will also have row size 1024 bytes and 64 rows
        const UINT tileRowBytes = 1024;
        const UINT numRowsPerTile = 64;

        UINT srcOffset = in_subresourceData.m_offset;

        // offset into this tile
        UINT startRow = in_coord.Y * numRowsPerTile;
        srcOffset += (in_subresourceData.m_rowPitch * startRow);
        srcOffset += in_coord.X * tileRowBytes;

        // copy the rows of this tile
        for (UINT row = 0; row < numRowsPerTile; row++) {
            memcpy(out_pDst, in_pSrc + srcOffset, tileRowBytes);

            out_pDst += tileRowBytes;
            srcOffset += in_subresourceData.m_rowPitch;
        }

        return XetFileHeader::GetTileSize();
    }

    void XetTextureGenerator::CompressTile(std::vector<BYTE>& inout_tile) {
        auto bound = mCompressor->CompressBufferBound(D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
        std::vector<BYTE> scratch(bound);

        size_t compressedDataSize = 0;
        HRESULT hr = mCompressor->CompressBuffer(
            inout_tile.data(), D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES,
            mCompressionLevel,
            scratch.data(), bound, &compressedDataSize);

        assert(compressedDataSize <= D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);

        scratch.resize(compressedDataSize);

        inout_tile.swap(scratch);
    }

    UINT XetTextureGenerator::WritePackedMips(BYTE* in_pBytes, size_t in_numBytes) {
        UINT numPackedMipBytes = 0;
        for (UINT i = 0; i < mXetHeader.m_mipInfo.m_numPackedMips; i++) {
            UINT s = mXetHeader.m_mipInfo.m_numStandardMips + i;
            mXetSubresourceInfos[s].m_packedMipInfo = XetFileHeader::PackedMipInfo{
                mSourceSubresourceDatas[s].m_rowPitch,
                mSourceSubresourceDatas[s].m_slicePitch,
                0xbaadbaad, 0xbaadbaad }; // FIXME: include padded row pitch and slice pitch
            numPackedMipBytes += mSourceSubresourceDatas[s].m_slicePitch;
        }

        // packed mip data is at the end of the DDS file
        UINT srcOffset = UINT(in_numBytes - numPackedMipBytes);

        BYTE* pSrc = &in_pBytes[srcOffset];
        PadPackedMips(pSrc);
        UINT numBytesPadded = (UINT)mPackedMipData.size(); // uncompressed and padded

        size_t numBytesCompressed = numBytesPadded; // unless we compress...
        if (mCompressionFormat) {
            // input to CompressBuffer() is a UINT32
            auto bound = mCompressor->CompressBufferBound(numBytesPadded);
            std::vector<BYTE> scratch(bound);

            HRESULT hr = mCompressor->CompressBuffer(
                mPackedMipData.data(), numBytesPadded,
                mCompressionLevel,
                scratch.data(), bound, &numBytesCompressed);
            scratch.resize(numBytesCompressed);
            mPackedMipData.swap(scratch);
        }

        // last offset structure points at the packed mips
        XetFileHeader::TileData outData{ 0 };
        outData.m_offset = mOffsets.back().m_offset + mOffsets.back().m_numBytes;
        outData.m_numBytes = (UINT32)numBytesCompressed;
        mOffsets.push_back(outData);

        return numBytesPadded;
    }

    void XetTextureGenerator::PadPackedMips(const BYTE* in_psrc) {
        UINT firstSubresource = mXetHeader.m_mipInfo.m_numStandardMips;
        UINT numSubresources = mXetHeader.m_mipInfo.m_numPackedMips;
        UINT64 totalBytes = 0;
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> srcLayout(numSubresources);
        std::vector<UINT> numRows(numSubresources);
        std::vector<UINT64> rowSizeBytes(numSubresources);
        
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        D3D12CreateDevice(0, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));

        device->GetCopyableFootprints(&mD3D12ResourceDesc, firstSubresource, numSubresources,
            0, srcLayout.data(), numRows.data(), rowSizeBytes.data(), &totalBytes);

        mPackedMipData.resize(totalBytes);

        BYTE* pDst = mPackedMipData.data();

        for (UINT i = 0; i < numSubresources; i++) {
            for (UINT r = 0; r < numRows[i]; r++) {
                memcpy(pDst, in_psrc, rowSizeBytes[i]);
                pDst += srcLayout[i].Footprint.RowPitch;
                in_psrc += rowSizeBytes[i];
            }
        }
    }

    UINT XetTextureGenerator::GetAlignedSize(UINT in_numBytes) {
        UINT alignment = 4096 - 1;
        UINT aligned = (in_numBytes + alignment) & (~alignment);
        return aligned;
    }

}