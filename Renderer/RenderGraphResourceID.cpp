#include "RenderGraphResourceID.h"
#include "Tools/Assert.h"

namespace Renderer {

	std::unordered_map<std::string, uint64_t> RenderGraphResourceID::mResourceNameToIDs;
	std::vector<std::string> RenderGraphResourceID::mIDToResourceNames;
	std::queue<uint64_t> RenderGraphResourceID::mRetiredIDs;

	RenderGraphResourceID::RenderGraphResourceID(uint64_t id)
	: mID(id) {}

	RenderGraphResourceID RenderGraphResourceID::FindOrCreateResourceID(const std::string& name) {
		if (mResourceNameToIDs.find(name) != mResourceNameToIDs.end()) {
			return RenderGraphResourceID{ mResourceNameToIDs.at(name) };
		}

		uint64_t id{ 0u };
		if (mRetiredIDs.empty()) {
			id = mIDToResourceNames.size();
			mIDToResourceNames.resize(id + 1u);
		}
		else {
			id = mRetiredIDs.front();
			mRetiredIDs.pop();
		}
		mIDToResourceNames.at(id) = name;
		mResourceNameToIDs[name] = id;

		return RenderGraphResourceID{ id };
	}

	void RenderGraphResourceID::RetireResourceID(const std::string& name) {
		if (mResourceNameToIDs.find(name) == mResourceNameToIDs.end()) return;

		uint64_t idToRetired = mResourceNameToIDs.at(name);
		mRetiredIDs.push(idToRetired);

		mResourceNameToIDs.erase(name);
	}


	SubresourceID EncodeSubresourceID(const RenderGraphResourceID& id, uint32_t subresourceIndex, bool isBuffer) {
		uint64_t resourceTypeFlag = ((uint64_t)isBuffer << 63);
		uint64_t subresourceID = (id.GetID() << 32) | subresourceIndex | resourceTypeFlag;
		return subresourceID;
	}

	std::tuple<RenderGraphResourceID, uint32_t, bool> DecodeSubresourceID(const SubresourceID& subresourceID) {
		uint64_t id = (subresourceID & 0x0FFFFFFFFFFFFFF) >> 32;
		uint32_t subresourceIndex = subresourceID & 0x00000000FFFFFFFF;
		bool isBuffer = (bool)(subresourceID & 0xF000000000000000);
		return std::make_tuple(RenderGraphResourceID{ id }, subresourceIndex, isBuffer);
	}

}