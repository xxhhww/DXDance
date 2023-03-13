#include "Texture.h"

namespace GHL {
	Texture::Texture(GpuDevice* device, const TextureDesc& textureDesc)
	: Resource(device)
	, mTextureDesc(textureDesc) {

		mResourceDesc.Dimension = GetD3DTextureDimension(mTextureDesc.dimension);
		mResourceDesc.Format = mTextureDesc.format;
		mResourceDesc.MipLevels = mTextureDesc.mipLevals;
		mResourceDesc.Alignment = 0;
		mResourceDesc.DepthOrArraySize = (mTextureDesc.dimension == ETextureDimension::Texture3D) ? mTextureDesc.depth : mTextureDesc.arraySize;
		mResourceDesc.Height = mTextureDesc.height;
		mResourceDesc.Width = mTextureDesc.width;
		mResourceDesc.SampleDesc.Count = mTextureDesc.sampleCount;
		mResourceDesc.SampleDesc.Quality = 0u;
		mResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 保留方式创建时，需要将其做修改
	}
}