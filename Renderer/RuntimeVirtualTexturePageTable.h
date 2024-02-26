#pragma once
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"

namespace Renderer {

	struct RuntimeVirtualTexturePageTableNodeRuntimeState {
	public:
		bool tempFlag{ false };

		bool inReady{ false };		// �Ƿ���CPU�˷��䵽һ��AtlasNode
		bool inQueue{ false };		// �Ƿ�λ�����������
		bool inLoading{ false };	// ��GPU������

		bool inReadyOut{ false };
		bool inQueueOut{ false };
		bool inLoadingOut{ false };

		bool inTexture{ false };	// ������ͼ����

		RuntimeVirtualTextureAtlasTileCache::Node* atlasNode{ nullptr };

		uint32_t pageLevel;

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