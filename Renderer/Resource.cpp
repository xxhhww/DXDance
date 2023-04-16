#include "Resource.h"

#include "Tools/VisitorHelper.h"

namespace Renderer {

	Resource::Resource(const GHL::Device* device, const ResourceFormat& resourceFormat)
	: mDevice(device)
	, mResourceFormat(resourceFormat) {}

	bool Resource::IsBuffer() const {
		bool isBuffer{ false };
		std::visit(MakeVisitor(
			[&](const TextureDesc& desc) {
				isBuffer = false;
			},
			[&](const BufferDesc& desc) {
				isBuffer = true;
			}
		), mResourceFormat.GetResourceDescVariant());

		return isBuffer;
	}

}