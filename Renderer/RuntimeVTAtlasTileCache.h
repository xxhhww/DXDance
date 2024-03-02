#pragma once
#include <vector>
#include "Math/Int.h"
#include "Math/Vector.h"

namespace Renderer {

	class RuntimeVTAtlasTileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID{ -1 };		// 在图集上的ID
			Math::Int2 tilePos{ -1, -1 };	// 在图集上的索引

			int32_t    pageLevel{ -1 };		// 所处页表级别
			Math::Int2 pagePos{ -1, -1 };	// 所处页表级别下的位置
			Math::Int2 pageOffset{ -1, -1 };
			Math::Int2 testpagePos{ -1, -1 };
			Math::Vector4 realRect;

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			inline ~Node() = default;
		};

	public:
		RuntimeVTAtlasTileCache(int32_t tileCountPerAxis);
		~RuntimeVTAtlasTileCache();

		// 添加到列表表尾
		void AddTail(RuntimeVTAtlasTileCache::Node* node);

		// 添加到列表表头
		void AddHead(RuntimeVTAtlasTileCache::Node* node);

		// 移除
		void Remove(RuntimeVTAtlasTileCache::Node* node);

		// 获取列表头节点（最不活跃节点）
		inline RuntimeVTAtlasTileCache::Node* GetHead() { return mHead; }

		// 获取列表尾节点（最活跃节点）
		inline RuntimeVTAtlasTileCache::Node* GetTail() { return mTail; }

	public:
		std::vector<RuntimeVTAtlasTileCache::Node*> mNodes;
		RuntimeVTAtlasTileCache::Node* mHead{ nullptr };
		RuntimeVTAtlasTileCache::Node* mTail{ nullptr };
	};

}