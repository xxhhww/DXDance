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
		* ֪ͨProcess�߳̽��д���
		*/
		void SetFrameCompletedEvent();

	private:

		/*
		* ProcessFeedback�߳�
		*/
		void ProcessFeedbackThread();

	private:
		TerrainSystem* mTerrainSystem{ nullptr };

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessFeedbackThread;
	};

}