#pragma once
#include "Resource.h"
#include "ResourceDesc.h"
#include "DescriptorHeap.h"

namespace GHL {
	
	class GpuDevice;

	class Texture : public Resource {
	public:
		Texture(GpuDevice* device, const TextureDesc& textureDesc);
		~Texture() = default;

		/*
		* Get·½·¨
		*/
		inline const auto& GetTextureDesc()             const { return mTextureDesc; }
		inline const auto& GetSRView()                  const { return mSRDescriptor; }
		inline const auto& GetDSView()                  const { return mDSDescriptor; }
		inline const auto& GetRTView()                  const { return mRTDescriptor; }
		inline const auto& GetUAView(uint32_t mipLevel) const { return mUADescriptors.at(mipLevel); }

	private:
		GpuDevice* mGpuDevice{ nullptr };
		TextureDesc mTextureDesc{};

		DescriptorHandle mSRDescriptor;
		DescriptorHandle mDSDescriptor;
		DescriptorHandle mRTDescriptor;
		std::vector<DescriptorHandle> mUADescriptors;
	};

}