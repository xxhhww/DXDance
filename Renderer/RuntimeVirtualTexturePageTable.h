#pragma once
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"

namespace Renderer {

	struct RuntimeVirtualTexturePageTableNodeRuntimeState {
	public:
		bool inReady{ false };		// �Ƿ���CPU�˷��䵽һ��AtlasNode
		bool inQueue{ false };		// �Ƿ�λ�����������
		bool inLoading{ false };	// ��GPU������
		bool inTexture{ false };	// ������ͼ����

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