#include "RenderGraphResourceID.h"
#include "Tools/Assert.h"

namespace Renderer {

	std::unordered_map<std::string, uint64_t> RenderGraphResourceID::mResourceNameToIDs;
	std::vector<std::string> RenderGraphResourceID::mIDToResourceNames;

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

	SubresourceID EncodeSubresourceID(const RenderGraphResourceID& id, uint32_t subresourceIndex, bool isBuffer) {
		uint64_t resourceTypeFlag = ((uint64_t)isBuffer << 63);
		return (id.GetID() << 32) | subresourceIndex | resourceTypeFlag;
	}

	std::tuple<RenderGraphResourceID, uint32_t, bool> DecodeSubresourceID(const SubresourceID& subresourceID) {
		uint64_t id = subresourceID >> 32;
		uint32_t subresourceIndex = subresourceID & 0x0000FFFF;
		bool isBuffer = (bool)(subresourceID & 0xF0000000);
		return std::make_tuple(RenderGraphResourceID{ id }, subresourceIndex, isBuffer);
	}

}