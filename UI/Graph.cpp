#include "Graph.h"
#include <queue>

namespace App {
	inline void Graph::PushNode(Node::Ptr node) {
		mNeighborMap[node->objectID] = std::vector<int>{};
		mNodeMap[node->objectID] = std::move(node);
	}

	inline void Graph::PushLink(Link& link) {
		// 删除冲突的Link
		std::vector<int> linkDeleted{};
		for (const auto& pair : mLinkMap) {
			if (link.toSlot == pair.second.toSlot) {
				linkDeleted.push_back(pair.second.objectID);
			}
		}
		for (const auto& id : linkDeleted) {
			EraseLink(id);
		}

		// 更新邻接表
		mNeighborMap[link.ToNodeID()].push_back(link.objectID);
		// 更新node的插槽
		mNodeMap[link.FromNodeID()]->SetOutputSlotLinked(link.FromSlotIndex());
		mNodeMap[link.ToNodeID()]->SetInputSlotLinked(link.ToSlotIndex(), link.objectID);
		// 更新linkMap
		mLinkMap[link.objectID] = std::move(link);

		// 更新Slot类型
		std::queue<Node*> tmpQueue{};
		tmpQueue.push(mNodeMap[link.ToNodeID()].get());
		while (!tmpQueue.empty()) {
			Node* currNode = tmpQueue.front();
			tmpQueue.pop();
			// 寻找输入的Pin
			std::vector<Slot*> oppoSlots;
			std::vector<int> inLinks = mNeighborMap[currNode->objectID];
			for (const int& linkID : inLinks) {
				const Link& inLink = mLinkMap[linkID];
				Slot& oppoSlot = mNodeMap[inLink.FromNodeID()]->GetOutputSlots().at(inLink.FromSlotIndex());
				oppoSlots.push_back(&oppoSlot);
			}
			if (currNode->OnInputSlotTypeChanged(oppoSlots)) {
				for (const auto& pair : mLinkMap) {
					if (pair.second.FromNodeID() == currNode->objectID) {
						tmpQueue.push(mNodeMap[pair.second.ToNodeID()].get());
					}
				}
			}
		}
	}

	inline void Graph::EraseNode(uint32_t id) {
		// 删除Link
		std::vector<uint32_t> linkDeleted{};
		for (const auto& pair : mLinkMap) {
			const Link& link = pair.second;
			if (link.FromNodeID() == id || link.ToNodeID() == id) {
				linkDeleted.push_back(link.objectID);
			}
		}
		for (const auto& id : linkDeleted) {
			EraseLink(id);
		}
		// 删除邻接
		mNeighborMap.erase(id);
		// 删除节点
		mNodeMap.erase(id);
	}

	inline void Graph::EraseLink(uint32_t id) {
		if (mLinkMap.find(id) == mLinkMap.end()) return;
		// 复制一份
		const Link link = mLinkMap[id];
		// 更新link两端节点的插槽
		mNodeMap[link.FromNodeID()]->SetOutputSlotBroken(link.FromSlotIndex());
		mNodeMap[link.ToNodeID()]->SetInputSlotBroken(link.ToSlotIndex());

		// 删除邻接
		uint32_t nodeID = link.ToNodeID();
		if (mNeighborMap.find(nodeID) == mNeighborMap.end()) return;
		auto& neighbors = mNeighborMap[nodeID];
		auto iter = std::find(neighbors.begin(), neighbors.end(), link.objectID);
		if (iter == neighbors.end()) return;
		neighbors.erase(iter);
		// 删除Link
		mLinkMap.erase(id);

		// 更新Slot类型
		std::queue<Node*> tmpQueue{};
		tmpQueue.push(mNodeMap[link.ToNodeID()].get());
		while (!tmpQueue.empty()) {
			Node* currNode = tmpQueue.front();
			tmpQueue.pop();
			// 寻找输入的Pin
			std::vector<Slot*> oppoSlots;
			std::vector<int> enterLinks = mNeighborMap[currNode->objectID];
			for (const int& linkID : enterLinks) {
				const Link& enterLink = mLinkMap[linkID];
				Slot& oppoSlot = mNodeMap[enterLink.FromNodeID()]->GetOutputSlots().at(enterLink.FromSlotIndex());
				oppoSlots.push_back(&oppoSlot);
			}
			if (currNode->OnInputSlotTypeChanged(oppoSlots)) {
				for (const auto& pair : mLinkMap) {
					if (pair.second.FromNodeID() == currNode->objectID) {
						tmpQueue.push(mNodeMap[pair.second.ToNodeID()].get());
					}
				}
			}
		}
	}

	inline void Graph::clear() {
		mNodeMap.clear();
		mLinkMap.clear();
		mNeighborMap.clear();
	}
}