#include "ResourceFormat.h"

namespace Renderer {

	ResourceFormat::ResourceFormat(const GHL::Device* device, const TextureDesc& desc)
	: mDevice(device)
	, mResourceDescVariant(desc) {}

	ResourceFormat::ResourceFormat(const GHL::Device* device, const BufferDesc& desc)
	: mDevice(device)
	, mResourceDescVariant(desc) {}

	void ResourceFormat::ResolveD3DResourceDesc() {

	}

	void ResourceFormat::QueryResourceAllocationInfo() {

	}

}