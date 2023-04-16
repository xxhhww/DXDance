#pragma once
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>

namespace Renderer {

	class RenderGraphResourceID {
	public:
		struct HashFunc {
		public:
			size_t operator()(const RenderGraphResourceID& a) const {
				return std::hash<uint64_t>()(a.mID);
			}
		};

	public:
		RenderGraphResourceID() = default;
		RenderGraphResourceID(uint64_t id);

		inline const auto& GetID()   const { return mID; }
		inline const auto& GetName() const { return mIDToResourceNames.at(mID); }

		bool operator==(const RenderGraphResourceID& a) const { return mID == a.mID; }

		static RenderGraphResourceID FindOrCreateResourceID(const std::string& name);
		static void RetireResourceID(const std::string& name);

	private:
		static std::unordered_map<std::string, uint64_t> mResourceNameToIDs;
		static std::vector<std::string> mIDToResourceNames;
		static std::queue<uint64_t> mRetiredIDs;

	private:
		uint64_t mID{ 0u };
	};

	using SubresourceID = uint64_t;

	SubresourceID EncodeSubresourceID(const RenderGraphResourceID& id, uint32_t subresourceIndex, bool isBuffer);

	std::tuple<RenderGraphResourceID, uint32_t, bool> DecodeSubresourceID(const SubresourceID& subresourceID);

}