#include "ResourceFormat.h"

namespace Renderer {

	ResourceFormat::ResourceFormat(const GHL::Device* device)
	: mDevice(device) {}

	ResourceFormat::ResourceFormat(const GHL::Device* device, const TextureDesc& desc)
	: mDevice(device)
	, mResourceDescVariant(desc) {
		Build();
	}

	ResourceFormat::ResourceFormat(const GHL::Device* device, const BufferDesc& desc)
	: mDevice(device)
	, mResourceDescVariant(desc) {
		Build();
	}

	void ResourceFormat::Build() {

	}

	void ResourceFormat::SetTextureDesc(const TextureDesc& desc) {
		mResourceDescVariant = desc;
	}

	void ResourceFormat::SetBufferDesc(const BufferDesc& desc) {
		mResourceDescVariant = desc;
	}

	void ResourceFormat::ResolveD3DResourceDesc() {

	}

	void ResourceFormat::QueryResourceAllocationInfo() {

	}

}