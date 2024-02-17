#pragma once
#include "Renderer/TerrainRenderer.h"

namespace Renderer {

	class RuntimeVirtualTextureBackend {
	public:
		RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVirtualTextureBackend();

	private:
		// 后台线程
		void BackendThread();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		// 线程同步变量
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// 地形数据(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
	};

}