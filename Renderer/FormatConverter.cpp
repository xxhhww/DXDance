#include "FormatConverter.h"

namespace Renderer {

	Renderer::TextureDesc FormatConverter::GetTextureDesc(const DirectX::TexMetadata& metadata) {
        Renderer::TextureDesc desc{};
        desc.dimension =
            metadata.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D ?
            GHL::ETextureDimension::Texture2D : GHL::ETextureDimension::Texture3D;
        desc.width = metadata.width;
        desc.height = metadata.height;
        desc.depth = metadata.depth;
        desc.arraySize = metadata.arraySize;
        desc.mipLevals = metadata.mipLevels;
        desc.sampleCount = 1u;
        desc.format = metadata.format;
        desc.usage = GHL::EResourceUsage::Default;
        desc.miscFlag = metadata.IsCubemap() ?
            GHL::ETextureMiscFlag::CubeTexture : GHL::ETextureMiscFlag::None;
        desc.createdMethod = GHL::ECreatedMethod::Committed;

        return desc;
	}

	DirectX::TexMetadata FormatConverter::GetTexMetadata(const Renderer::TextureDesc& textureDesc) {
        DirectX::TexMetadata metadata{};
        metadata.width = textureDesc.width;
        metadata.height = textureDesc.height;
        metadata.depth = textureDesc.depth;
        metadata.arraySize = textureDesc.arraySize;
        metadata.mipLevels = textureDesc.mipLevals;
        metadata.miscFlags =
            HasAnyFlag(textureDesc.miscFlag, GHL::ETextureMiscFlag::CubeTexture) ?
            DirectX::TEX_MISC_TEXTURECUBE : 0u;
        metadata.miscFlags2;
        metadata.format = textureDesc.format;
        metadata.dimension =
            (textureDesc.dimension == GHL::ETextureDimension::Texture2D) ?
            DirectX::TEX_DIMENSION_TEXTURE2D : DirectX::TEX_DIMENSION_TEXTURE3D;

        return metadata;
	}

}