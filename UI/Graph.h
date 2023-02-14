#pragma once
#include "Node.h"

namespace UI {
	class Graph {
	public:
		void PushNode(Node::Ptr node);
		void PushLink(Link& link);

		void EraseNode(uint32_t id);
		void EraseLink(uint32_t id);

		void clear();

		inline auto& GetNodeMap()		{ return mNodeMap; }
		inline auto& GetLinkMap()		{ return mLinkMap; }
		inline auto& GetNeighborMap()	{ return mNeighborMap; }

		inline const auto& GetNodeMap()		const { return mNodeMap; }
		inline const auto& GetLinkMap()		const { return mLinkMap; }
		inline const auto& GetNeighborMap()	const { return mNeighborMap; }
	private:
		std::unordered_map<int, Node::Ptr> mNodeMap;
		std::unordered_map<int, Link> mLinkMap;
		// first: node id, second: link id
		std::unordered_map<int, std::vector<int>> mNeighborMap;
	};
}