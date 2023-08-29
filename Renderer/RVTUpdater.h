#pragma once
#include <Windows.h>
#include <thread>

namespace Renderer {

	class TerrainSystem;

	class RVTUpdater {
	public:
		RVTUpdater(TerrainSystem* terrainSystem);
		~RVTUpdater();

		/*
		* 通知Process线程进行处理
		*/
		void SetFrameCompletedEvent();

	private:

		/*
		* ProcessFeedback线程
		*/
		void ProcessFeedbackThread();

	private:
		TerrainSystem* mTerrainSystem{ nullptr };

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessFeedbackThread;
	};

}