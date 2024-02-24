#pragma once
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"

namespace Renderer {

	struct RuntimeVirtualTexturePageTableNodeRuntimeState {
	public:
		bool inReady{ false };		// 是否在CPU端分配到一个AtlasNode
		bool inQueue{ false };		// 是否位于任务队列中
		bool inLoading{ false };	// 在GPU加载中
		bool inTexture{ false };	// 在纹理图集中

		RuntimeVirtualTextureAtlasTileCache::Node* atlasNode{ nullptr };

		uint32_t pageLevel;

	public:
		inline void SetInActive()  { inReady = false; inQueue = false; inLoading = false; inTexture = false; }
		inline void SetInReady()   { inReady = true;  inQueue = false; inLoading = false; inTexture = false; }
		inline void SetInQueue()   { inReady = false; inQueue = true;  inLoading = false; inTexture = false; }
		inline void SetInLoading() { inReady = false; inQueue = false; inLoading = true;  inTexture = false; }
		inline void SetInTexture() { inReady = false; inQueue = false; inLoading = false; inTexture = true; }
	};

	class RuntimeVirtualTexturePageTable {
	public:
		RuntimeVirtualTexturePageTable(uint32_t pageLevel, uint32_t tableSizeInPage0Level);
		~RuntimeVirtualTexturePageTable() = default;

		inline auto& GetNodeRuntimeState(uint32_t pagePosX, uint32_t pagePosY) { return mNodeRuntimeStates[pagePosX][pagePosY]; }

	private:
		uint32_t mPageLevel;
		uint32_t mNodeCountPerAxis;
		std::vector<std::vector<RuntimeVirtualTexturePageTableNodeRuntimeState>> mNodeRuntimeStates;
	};

}