#pragma once
#include "Renderer/TerrainRenderer.h"

namespace Renderer {

	/*
	* 后台类(负责创建与管理地形节点的全量表、流入流出地形节点数据、并与GPU驻留的地形数据做交互)
	*/
	class TerrainBackend {
	public:
		TerrainBackend(TerrainRenderer* renderer);
		~TerrainBackend();

		// 初始化
		void Initialize();

	private:
		// 后台线程
		void BackendThread();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		// 线程同步变量
		std::thread mThread;
		HANDLE mEvent;
		bool mThreadRunning{ true };

		// CopyQueue/CopyFence
		IDStorageQueue* mDStorageFileQueue{ nullptr };
		GHL::Fence* mCopyFence{ nullptr };

		// 延迟剔除器
		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 3u);
			~EvictionDelay();

			// 将当前帧需要剔除的四叉树节点累加起来
			void MoveToNextFrame();
			void Clear();

			// 重新评估本来需要剔除的四叉树节点
			void Rescue(std::vector<TerrainNodeID>& nodeIDs);

			inline void Append(const TerrainNodeID& nodeID) { mEvictionsBuffer[0].push_back(nodeID); }
			inline void Append(const std::vector<TerrainNodeID>& nodeIDs) { mEvictionsBuffer[0].insert(mEvictionsBuffer[0].end(), nodeIDs.begin(), nodeIDs.end()); }
			inline std::vector<TerrainNodeID>& GetReadyToEvict() { return mEvictionsBuffer.back(); }

		private:
			std::vector<std::vector<TerrainNodeID>> mEvictionsBuffer;
		};
		EvictionDelay mEvictionDelay;
	};

}