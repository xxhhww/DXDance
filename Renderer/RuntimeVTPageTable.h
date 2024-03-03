#pragma once
#include "Renderer/RuntimeVTAtlasTileCache.h"
#include "Math/Int.h"

namespace Renderer {

	struct RuntimeVTPageTableNodeRuntimeState {
	public:
		bool tempFlag{ false };

		bool inReady{ false };		// 是否在CPU端分配到一个AtlasNode
		bool inQueue{ false };		// 是否位于任务队列中
		bool inLoading{ false };	// 在GPU加载中

		bool inReadyOut{ false };
		bool inQueueOut{ false };
		bool inLoadingOut{ false };

		bool inTexture{ false };	// 在纹理图集中

		RuntimeVTAtlasTileCache::Node* atlasNode{ nullptr };

		Math::Int4 rectInPage0Level{ 0, 0,0 ,0 };

		int32_t  physicalPosX{ -1 };
		int32_t  physicalPosY{ -1 };
		uint32_t pageLevel{ 0u };

	public:
		inline void SetTempFlag(bool flag) { tempFlag = flag; }

		inline void SetInReady()   { inReady = true;  inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }
		inline void SetInQueue()   { inReady = false; inQueue = true;  inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }
		inline void SetInLoading() { inReady = false; inQueue = false; inLoading = true;  inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }

		inline void SetInReadyOut()   { inReady = false; inQueue = false; inLoading = false; inReadyOut = true;  inQueueOut = false; inLoadingOut = false; inTexture = true; }
		inline void SetInQueueOut()   { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = true;  inLoadingOut = false; inTexture = true; }
		inline void SetInLoadingOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = true;  inTexture = true; }

		inline void SetInTexture()  { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inTexture = true; }
		inline void SetOutTexture() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inTexture = false; }
	};

	class RuntimeVTPageTable {
	public:
		RuntimeVTPageTable(uint32_t pageLevel, uint32_t tableSizeInPage0Level);
		~RuntimeVTPageTable() = default;

		inline const auto& GetPageOffset() const { return mPageOffset; }

		void OnRuntimeVTRealRectChanged(Math::Int2 offsetInPage0Level, std::vector<RuntimeVTPageTableNodeRuntimeState*>& changedPageNodeRuntimeStates);

		RuntimeVTPageTableNodeRuntimeState& GetNodeRuntimeStateTransformed(int32_t pagePosX, int32_t pagePosY);

		RuntimeVTPageTableNodeRuntimeState& GetNodeRuntimeStateDirected(int32_t pagePosX, int32_t pagePosY);

		inline auto& GetNodeRuntimeStates() { return mNodeRuntimeStates; }
		inline const auto& GetNodeRuntimeStates() const { return mNodeRuntimeStates; }

		Math::Int2 GetTransformedXY(int32_t x, int32_t y);

	private:
		uint32_t mPageLevel{ 0u };
		int32_t  mNodeCountPerAxis{ 0u };
		Math::Int2 mPageOffset{ 0, 0 };
		std::vector<std::vector<RuntimeVTPageTableNodeRuntimeState>> mNodeRuntimeStates;
	};

}