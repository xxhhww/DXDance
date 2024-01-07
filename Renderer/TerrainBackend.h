#pragma once
#include "Renderer/TerrainRenderer.h"

namespace Renderer {

	/*
	* ��̨��(���𴴽��������νڵ��ȫ���������������νڵ����ݡ�����GPUפ���ĵ�������������)
	*/
	class TerrainBackend {
	public:
		TerrainBackend(TerrainRenderer* renderer);
		~TerrainBackend();

		// ��ʼ��
		void Initialize();

	private:
		// ��̨�߳�
		void BackendThread();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mEvent;
		bool mThreadRunning{ true };

		// CopyQueue/CopyFence
		IDStorageQueue* mDStorageFileQueue{ nullptr };
		GHL::Fence* mCopyFence{ nullptr };

		// �ӳ��޳���
		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 3u);
			~EvictionDelay();

			// ����ǰ֡��Ҫ�޳����Ĳ����ڵ��ۼ�����
			void MoveToNextFrame();
			void Clear();

			// ��������������Ҫ�޳����Ĳ����ڵ�
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