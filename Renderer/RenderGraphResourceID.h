#pragma once
#include <vector>
#include <unordered_map>
#include <string>

namespace Renderer {

	class RenderGraphResourceID {
	public:
		RenderGraphResourceID(uint64_t id);

		inline const auto& GetID()   const { return mID; }
		inline const auto& GetName() const { return mIDToResourceNames.at(mID); }

		static RenderGraphResourceID FindOrCreateResourceID(const std::string& name);

	private:
		static std::unordered_map<std::string, uint64_t> mResourceNameToIDs;
		static std::vector<std::string> mIDToResourceNames;

	private:
		uint64_t mID{ 0u };
	};

	std::unordered_map<std::string, uint64_t> RenderGraphResourceID::mResourceNameToIDs;
	std::vector<std::string> RenderGraphResourceID::mIDToResourceNames;

	using SubresourceID = uint64_t;

	SubresourceID EncodeSubresourceID(const RenderGraphResourceID& id, uint32_t subresourceIndex, bool isBuffer);

	std::tuple<RenderGraphResourceID, uint32_t, bool> DecodeSubresourceID(const SubresourceID& subresourceID);

}