#pragma once
#include "Node.h"
#include "Link.h"
#include <unordered_map>

namespace App {
	class Graph {
	public:
		void PushNode(std::unique_ptr<Node>& node);
		void PushLink(std::unique_ptr<Link>& link);

		void EraseNode(uint32_t id);
		void EraseLink(uint32_t id);

		void Clear();

		inline auto& GetNodeMap()		{ return mNodeMap; }
		inline auto& GetLinkMap()		{ return mLinkMap; }
		inline auto& GetNeighborMap()	{ return mNeighborMap; }

		inline const auto& GetNodeMap()		const { return mNodeMap; }
		inline const auto& GetLinkMap()		const { return mLinkMap; }
		inline const auto& GetNeighborMap()	const { return mNeighborMap; }
	private:
		std::unordered_map<int, std::unique_ptr<Node>> mNodeMap;
		std::unordered_map<int, std::unique_ptr<Link>> mLinkMap;
		// first: node id, second: link id
		std::unordered_map<int, std::vector<int>> mNeighborMap;
	};
}