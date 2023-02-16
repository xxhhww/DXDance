#include "Graph.h"
#include <queue>

namespace App {
	void Graph::PushNode(std::unique_ptr<Node>& node) {
		mNeighborMap[node->objectID] = std::vector<int>{};
		mNodeMap[node->objectID] = std::move(node);
	}

	void Graph::PushLink(std::unique_ptr<Link>& link) {
		// ɾ����ͻ��Link
		std::vector<int> linkDeleted{};
		for (const auto& pair : mLinkMap) {
			if (link->endPin == pair.second->endPin) {
				linkDeleted.push_back(pair.second->objectID);
			}
		}
		for (const auto& id : linkDeleted) {
			EraseLink(id);
		}

		// �����ڽӱ�
		mNeighborMap[link->EndNodeID()].push_back(link->objectID);
		// ����node�Ĳ��
		mNodeMap[link->StartNodeID()]->SetOutputPinLinked(link->StartPinIndex());
		mNodeMap[link->EndNodeID()]->SetInputPinLinked(link->EndPinIndex(), link->objectID);
		// ����linkMap
		int endNodeID = link->EndNodeID();
		mLinkMap[link->objectID] = std::move(link);

		// ����Slot����
		std::queue<Node*> tmpQueue{};
		tmpQueue.push(mNodeMap[endNodeID].get());
		while (!tmpQueue.empty()) {
			Node* currNode = tmpQueue.front();
			tmpQueue.pop();
			// Ѱ�������Pin
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
		// ɾ��Link
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
		// ɾ���ڽ�
		mNeighborMap.erase(id);
		// ɾ���ڵ�
		mNodeMap.erase(id);
	}

	void Graph::EraseLink(uint32_t id) {
		if (mLinkMap.find(id) == mLinkMap.end()) return;
		// ����һ��
		const auto& link = mLinkMap[id];
		// ����link���˽ڵ�Ĳ��
		mNodeMap[link->StartNodeID()]->SetOutputPinBroken(link->StartPinIndex());
		mNodeMap[link->EndNodeID()]->SetInputPinBroken(link->EndPinIndex());

		// ɾ���ڽ�
		uint32_t nodeID = link->EndNodeID();
		if (mNeighborMap.find(nodeID) == mNeighborMap.end()) return;
		auto& neighbors = mNeighborMap[nodeID];
		auto iter = std::find(neighbors.begin(), neighbors.end(), link->objectID);
		if (iter == neighbors.end()) return;
		neighbors.erase(iter);
		// ɾ��Link
		mLinkMap.erase(id);

		// ����Slot����
		std::queue<Node*> tmpQueue{};
		tmpQueue.push(mNodeMap[link->EndNodeID()].get());
		while (!tmpQueue.empty()) {
			Node* currNode = tmpQueue.front();
			tmpQueue.pop();
			// Ѱ�������Pin
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