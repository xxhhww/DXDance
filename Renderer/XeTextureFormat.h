#pragma once
#include "GHL/DDS.h"
#include "ResourceFormat.h"

#include <vector>

namespace Renderer {

	struct XetFileHeader {
	public:
        static UINT GetMagic() { return 0x20544558; }
        static UINT GetTileSize() { return 65536; } // uncompressed size
        static UINT GetVersion() { return 3; }

        UINT m_magic{ GetMagic() };
        UINT m_version{ GetVersion() };
        DirectX::DDS_HEADER m_ddsHeader;
        DirectX::DDS_HEADER_DXT10 m_extensionHeader;

        UINT32 m_compressionFormat{ 0 }; // 0 is no compression

        struct MipInfo {
            UINT32 m_numStandardMips;
            UINT32 m_numTilesForStandardMips; // the TileData[] array has # entries = (m_numTilesForStandardMips + 1)
            UINT32 m_numPackedMips;
            UINT32 m_numTilesForPackedMips;   // only 1 entry for all packed mips at TileData[m_numTilesForStandardMips]
            UINT32 m_numUncompressedBytesForPackedMips; // if this equals the size at TileData[m_numTilesForStandardMips], then not compressed
        };
        MipInfo m_mipInfo;

        // use subresource tile dimensions to generate linear tile index
        struct StandardMipInfo {
            UINT32 m_widthTiles;
            UINT32 m_heightTiles;
            UINT32 m_depthTiles;

            // convenience value, can be computed from sum of previous subresource dimensions
            UINT32 m_subresourceTileIndex;
        };

        // properties of the uncompressed packed mips
        // all packed mips are padded and treated as a single entity
        struct PackedMipInfo {
            UINT32 m_rowPitch;   // before padding
            UINT32 m_slicePitch; // before padding

            UINT32 m_rowPitchPadded;   // after padding, from footprint
            UINT32 m_slicePitchPadded; // after padding, from footprint
        };

        // array SubresourceInfo[m_ddsHeader.mipMapCount]
        struct SubresourceInfo {
            union {
                StandardMipInfo m_standardMipInfo;
                PackedMipInfo m_packedMipInfo;
            };
        };

        // array TileData[m_numTilesForStandardMips + 1], 1 entry for each tile plus a final entry for packed mips
        struct TileData {
            UINT32 m_offset;          // file offset to tile data
            UINT32 m_numBytes;        // # bytes for the tile
        };

        // arrays for file lookup start after sizeof(XetFileHeader)
        // 1st: array SubresourceInfo[m_ddsHeader.mipMapCount]
        // 2nd: array TileData[m_numTilesForStandardMips + 1]
        // 3rd: packed mip data can be found at TileData[m_numTilesForStandardMips].m_offset TileData[m_numTilesForStandardMips].m_numBytes
	};

	class XeTexureFormat {
    public:
        std::string GetFilename() const { return m_filename; }
        DXGI_FORMAT GetFormat() const { return m_fileHeader.m_extensionHeader.dxgiFormat; }
        UINT GetImageWidth() const { return m_fileHeader.m_ddsHeader.width; }
        UINT GetImageHeight() const { return m_fileHeader.m_ddsHeader.height; }
        UINT GetMipCount() const { return m_fileHeader.m_ddsHeader.mipMapCount; }
        UINT32 GetCompressionFormat() const { return m_fileHeader.m_compressionFormat; }

        // return value is # bytes. out_offset is byte offset into file
        struct FileOffset { UINT offset{ 0 }; UINT numBytes{ 0 }; };
        FileOffset GetFileOffset(const D3D12_TILED_RESOURCE_COORDINATE& in_coord) const;

        UINT GetPackedMipFileOffset(UINT* out_pNumBytesTotal, UINT* out_pNumBytesUncompressed) const;

        TextureDesc ConvertTextureDesc() const;

        XeTexureFormat(const std::string& in_filename);
    protected:
        static const UINT MIN_STRIDE_BYTES{ 256 };
        static const UINT NUM_BYTES_PER_TILE{ D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES }; // tiles are always 64KB in size

        std::string m_filename;
        XetFileHeader m_fileHeader;

        std::vector<XetFileHeader::SubresourceInfo> m_subresourceInfo;
        std::vector<XetFileHeader::TileData> m_tileOffsets;

        UINT GetLinearIndex(const D3D12_TILED_RESOURCE_COORDINATE& in_coord) const;
	};

}