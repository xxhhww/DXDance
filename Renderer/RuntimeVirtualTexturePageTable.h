#pragma once
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"

namespace Renderer {

	struct RuntimeVirtualTexturePageTableNodeRuntimeState {
	public:
		bool inQueue{ false };		// 是否位于任务队列中
		bool inLoading{ false };	// 在GPU加载中
		bool inTexture{ false };	// 在纹理图集中

		RuntimeVirtualTextureAtlasTileCache::Node* atlasNode{ nullptr };

		uint32_t pageLevel;

	public:
		inline void SetInActive() { inQueue = false; inLoading = false; inTexture = false; }
		inline void SetInQueue() { inQueue = true;  inLoading = false; inTexture = false; }
		inline void SetInLoading() { inQueue = false; inLoading = true;  inTexture = false; }
		inline void SetInTexture() { inQueue = false; inLoading = false; inTexture = true; }
	};

	class RuntimeVirtualTexturePageTable {
	public:
		RuntimeVirtualTexturePageTable(uint32_t pageLevel, uint32_t tableSizeInPage0Level);
		~RuntimeVirtualTexturePageTable() = default;

		inline auto& GetNodeRuntimeState(Math::Int2 pagePos) { return mNodeRuntimeStates[pagePos.x][pagePos.y]; }

	private:
		uint32_t mPageLevel;
		uint32_t mNodeCountPerAxis;
		std::vector<std::vector<RuntimeVirtualTexturePageTableNodeRuntimeState>> mNodeRuntimeStates;
	};

}