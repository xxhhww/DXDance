#include "Graph.h"
#include <queue>

namespace App {
	void Graph::PushNode(std::unique_ptr<Node>& node) {
		mNeighborMap[node->objectID] = std::vector<int>{};
		mNodeMap[node->objectID] = std::move(node);
	}

	void Graph::PushLink(std::unique_ptr<Link>& link) {
		// 删除冲突的Link
		std::vector<int> linkDeleted{};
		for (const auto& pair : mLinkMap) {
			if (link->endPin == pair.second->endPin) {
				linkDeleted.push_back(pair.second->objectID);
			}
		}
		for (const auto& id : linkDeleted) {
			EraseLink(id);
		}

		// 更新邻接表
		mNeighborMap[link->EndNodeID()].push_back(link->objectID);
		// 更新node的插槽
		mNodeMap[link->StartNodeID()]->SetOutputPinLinked(link->StartPinIndex());
		mNodeMap[link->EndNodeID()]->SetInputPinLinked(link->EndPinIndex(), link->objectID);
		// 更新linkMap
		int endNodeID = link->EndNodeID();
		mLinkMap[link->objectID] = std::move(link);

		// 更新Slot类型
		std::queue<Node*> tmpQueue{};
		tmpQueue.push(mNodeMap[endNodeID].get());
		while (!tmpQueue.empty()) {
			Node* currNode = tmpQueue.front();
			tmpQueue.pop();
			// 寻找输入的Pin
			std::vector<Pin*> oppoSlots;
			std::vector<int> inLinks = mNeighborMap[currNode->objectID];
			for (const int& linkID : inLinks) {
				const auto& inLink = mLinkMap[linkID];
				Pin* oppoSlot = mNodeMap[inLink->StartNodeID()]->GetOutputPins().at(inLink->StartPinIndex());
				oppoSlots.push_back(oppoSlot);
			}
			if (currNode->OnInputPinTypeChanged(oppoSlots)) {
				for (const auto& pair : mLinkMap) {
					if (pair.second->StartNodeID() == currNode->objectID) {
						tmpQueue.push(mNodeMap[pair.second->EndNodeID()].get());
					}
				}
			}
		}
	}

	void Graph::EraseNode(uint32_t id) {
		// 删除Link
		std::vector<uint32_t> linkDeleted{};
		for (const auto& pair : mLinkMap) {
			const auto& link = pair.second;
			if (link->StartNodeID() == id || link->EndNodeID() == id) {
				linkDeleted.push_back(link->objectID);
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

	void Graph::EraseLink(uint32_t id) {
		if (mLinkMap.find(id) == mLinkMap.end()) return;
		// 复制一份
		const auto& link = mLinkMap[id];
		// 更新link两端节点的插槽
		mNodeMap[link->StartNodeID()]->SetOutputPinBroken(link->StartPinIndex());
		mNodeMap[link->EndNodeID()]->SetInputPinBroken(link->EndPinIndex());

		// 删除邻接
		uint32_t nodeID = link->EndNodeID();
		if (mNeighborMap.find(nodeID) == mNeighborMap.end()) return;
		auto& neighbors = mNeighborMap[nodeID];
		auto iter = std::find(neighbors.begin(), neighbors.end(), link->objectID);
		if (iter == neighbors.end()) return;
		neighbors.erase(iter);
		// 删除Link
		mLinkMap.erase(id);

		// 更新Slot类型
		std::queue<Node*> tmpQueue{};
		tmpQueue.push(mNodeMap[link->EndNodeID()].get());
		while (!tmpQueue.empty()) {
			Node* currNode = tmpQueue.front();
			tmpQueue.pop();
			// 寻找输入的Pin
			std::vector<Pin*> oppoSlots;
			std::vector<int> enterLinks = mNeighborMap[currNode->objectID];
			for (const int& linkID : enterLinks) {
				const auto& enterLink = mLinkMap[linkID];
				Pin* oppoSlot = mNodeMap[enterLink->StartNodeID()]->GetOutputPins().at(enterLink->StartPinIndex());
				oppoSlots.push_back(oppoSlot);
			}
			if (currNode->OnInputPinTypeChanged(oppoSlots)) {
				for (const auto& pair : mLinkMap) {
					if (pair.second->StartNodeID() == currNode->objectID) {
						tmpQueue.push(mNodeMap[pair.second->EndNodeID()].get());
					}
				}
			}
		}
	}

	void Graph::Clear() {
		mNodeMap.clear();
		mLinkMap.clear();
		mNeighborMap.clear();
	}
}