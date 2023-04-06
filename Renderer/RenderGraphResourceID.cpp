#include "RenderGraphResourceID.h"
#include "Tools/Assert.h"

namespace Renderer {

	RenderGraphResourceID::RenderGraphResourceID(uint64_t id)
	: mID(id) {}

	RenderGraphResourceID RenderGraphResourceID::FindOrCreateResourceID(const std::string& name) {
		if (mResourceNameToIDs.find(name) != mResourceNameToIDs.end()) {
			return RenderGraphResourceID{ mResourceNameToIDs.at(name) };
		}

		uint64_t id = mIDToResourceNames.size();
		mIDToResourceNames.push_back(name);
		mResourceNameToIDs[name] = id;

		return RenderGraphResourceID{ id };
	}

	SubresourceID EncodeSubresourceID(const RenderGraphResourceID& id, uint32_t subresourceIndex) {
		return (id.GetID() << 32) | subresourceIndex;
	}

	std::pair<RenderGraphResourceID, uint32_t> DecodeSubresourceID(const SubresourceID& subresourceID) {
		uint64_t id = subresourceID >> 32;
		uint32_t subresourceIndex = subresourceID & 0x0000FFFF;
		return std::make_pair(RenderGraphResourceID{ id }, subresourceIndex);
	}

}