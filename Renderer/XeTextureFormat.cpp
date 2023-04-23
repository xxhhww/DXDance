#include "XeTextureFormat.h"

#include "Tools/Assert.h"

#include <fstream>

namespace Renderer {

    TextureDesc XeTexureFormat::ConvertTextureDesc() const {
        TextureDesc desc{};
        desc.width = GetImageWidth();
        desc.height = GetImageHeight();
        desc.mipLevals = GetMipCount();
        desc.format = GetFormat();
        desc.supportStream = true;
        desc.initialState = GHL::EResourceState::PixelShaderAccess;
        desc.expectedState = desc.initialState;

        return desc;
    }

    XeTexureFormat::FileOffset XeTexureFormat::GetFileOffset(const D3D12_TILED_RESOURCE_COORDINATE& in_coord) const {
        // use index to look up file offset and number of bytes
        UINT index = GetLinearIndex(in_coord);
        FileOffset fileOffset;
        fileOffset.numBytes = m_tileOffsets[index].m_numBytes;
        fileOffset.offset = m_tileOffsets[index].m_offset;
        return fileOffset;
    }

    UINT XeTexureFormat::GetPackedMipFileOffset(UINT* out_pNumBytesTotal, UINT* out_pNumBytesUncompressed) const {
        UINT packedOffset = m_tileOffsets[m_fileHeader.m_mipInfo.m_numTilesForStandardMips].m_offset;
        *out_pNumBytesTotal = m_tileOffsets[m_fileHeader.m_mipInfo.m_numTilesForStandardMips].m_numBytes;
        *out_pNumBytesUncompressed = m_fileHeader.m_mipInfo.m_numUncompressedBytesForPackedMips;
        return packedOffset;
    }

    XeTexureFormat::XeTexureFormat(const std::string& in_filename) 
    : m_filename(in_filename) {
        std::ifstream inFile(in_filename.c_str(), std::ios::binary);
        ASSERT_FORMAT(!inFile.fail(), "File Not Exists");

        inFile.read((char*)&m_fileHeader, sizeof(m_fileHeader));
        ASSERT_FORMAT(inFile.good(), "Unexpected Error reading header");

        if (m_fileHeader.m_magic != XetFileHeader::GetMagic()) { ASSERT_FORMAT(false, "Not a valid XET file"); }
        if (m_fileHeader.m_version != XetFileHeader::GetVersion()) { ASSERT_FORMAT(false, "Incorrect XET version"); }

        m_subresourceInfo.resize(m_fileHeader.m_ddsHeader.mipMapCount);
        inFile.read((char*)m_subresourceInfo.data(), m_subresourceInfo.size() * sizeof(m_subresourceInfo[0]));
        if (!inFile.good()) { ASSERT_FORMAT(false, "Unexpected Error reading subresource info"); }

        m_tileOffsets.resize(m_fileHeader.m_mipInfo.m_numTilesForStandardMips + 1); // plus 1 for the packed mips offset & size
        inFile.read((char*)m_tileOffsets.data(), m_tileOffsets.size() * sizeof(m_tileOffsets[0]));
        if (!inFile.good()) { ASSERT_FORMAT(false, "Unexpected Error reading packed mip info"); }
    }

    UINT XeTexureFormat::GetLinearIndex(const D3D12_TILED_RESOURCE_COORDINATE& in_coord) const {
        const auto& data = m_subresourceInfo[in_coord.Subresource].m_standardMipInfo;
        return data.m_subresourceTileIndex + (in_coord.Y * data.m_widthTiles) + in_coord.X;
    }

}