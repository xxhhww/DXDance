#pragma once
#include <string>
#include <Windows/Window.h>
#include <DirectStorage/dstorage.h>
#include "Renderer/XeTextureFormat.h"

namespace Renderer {

	class XetTextureGenerator {
	public:
		struct SourceSubResourceData {
			UINT m_offset;
			UINT m_rowPitch;
			UINT m_slicePitch;
		};

	public:
		void Generate(const std::string& inFilePathname, const std::string& outputFilePathname);

	private:
		void GetTiling();

		// find the base address of each mip level
		void FillSubresourceData();

		void WriteTiles(const BYTE* in_pSrc);

		UINT WriteTile(BYTE* out_pDst,
			const D3D12_TILED_RESOURCE_COORDINATE& in_coord,
			const SourceSubResourceData& in_subresourceData,
			const BYTE* in_pSrc);

		void CompressTile(std::vector<BYTE>& inout_tile);

		UINT WritePackedMips(BYTE* in_pBytes, size_t in_numBytes);

		void PadPackedMips(const BYTE* in_psrc);

		UINT GetAlignedSize(UINT in_numBytes);

	private:
		static DXGI_FORMAT GetFormatFromHeader(const DirectX::DDS_HEADER& in_ddsHeader);

	private:
		uint32_t mCompressionFormat = 1u;
		XetFileHeader mXetHeader{};
		std::vector<XetFileHeader::SubresourceInfo> mXetSubresourceInfos;
		D3D12_RESOURCE_DESC mD3D12ResourceDesc{};

		std::vector<SourceSubResourceData> mSourceSubresourceDatas; // read from source DDS

		std::vector<XetFileHeader::TileData> mOffsets;	// offsets table
		std::vector<BYTE> mTextureData;	// texture bytes
		std::vector<BYTE> mPackedMipData;	// packed mip bytes

		Microsoft::WRL::ComPtr<IDStorageCompressionCodec> mCompressor;
		DSTORAGE_COMPRESSION mCompressionLevel = DSTORAGE_COMPRESSION_BEST_RATIO;
	};

}