#pragma once
#include <vector>

namespace Renderer {

    class LruCache {
    public:
        struct NodeInfo {
            inline NodeInfo(int id) : id(id) {}

            int id{ 0 };
            NodeInfo* next{ nullptr };
            NodeInfo* prev{ nullptr };
        };

        std::vector<NodeInfo*> allNodes;
        NodeInfo* head{ nullptr };
        NodeInfo* tail{ nullptr };

        inline const auto& GetFirst() const { return head->id; }

        void Create(int count) {
            allNodes.resize(count);

            for (int i = 0; i < count; i++) {
                allNodes[i] = new NodeInfo(i);
            }
            for (int i = 0; i < count; i++) {
                allNodes[i]->next = (i + 1 < count) ? allNodes[i + 1] : nullptr;
                allNodes[i]->prev = (i != 0) ? allNodes[i - 1] : nullptr;
            }
            head = allNodes[0];
            tail = allNodes[count - 1];
        }

        void Destory() {
            for (uint32_t i = 0; i < allNodes.size(); i++) {
                delete allNodes[i];
            }
            allNodes.clear();
        }

        bool SetActive(int id) {
            if (id < 0 || id >= allNodes.size()) {
                return false;
            }

            auto* node = allNodes[id];
            if (node == tail) {
                return true;
            }

            Remove(node);
            AddLast(node);
            return true;
        }

        void AddLast(NodeInfo* node) {
            auto* lastTail = tail;
            lastTail->next = node;
            tail = node;
            node->prev = lastTail;
        }

        void Remove(NodeInfo* node) {
            if (head == node) {
                head = node->next;
            }
            else {
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }
        }
    };

}