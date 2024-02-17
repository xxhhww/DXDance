#pragma once
#include "Renderer/TerrainRenderer.h"

namespace Renderer {

	class RuntimeVirtualTextureBackend {
	public:
		RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVirtualTextureBackend();

	private:
		// ��̨�߳�
		void BackendThread();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// ��������(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
	};

}