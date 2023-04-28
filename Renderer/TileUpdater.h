#pragma once
#include "StreamTexture.h"
#include <Windows.h>
#include <thread>

namespace GHL {
	class Device;
}

namespace Renderer {

	class RingFrameTracker;
	class StreamTextureManger;
	class DataUploader;

	/*
	* 磁贴更新器，在一个渲染帧完成后，负责更新纹理使用的磁贴，并更新全局ResidencyMap(用于指示纹理在显存中保留的最细节的MipLevel)
	*/
	class TileUpdater {
	public:
		TileUpdater(
			const GHL::Device* device, 
			RingFrameTracker* frameTracker, 
			std::unordered_map<std::string, std::unique_ptr<StreamTexture>>* textureStorage, 
			DataUploader* dataUploader);
		~TileUpdater();

		/*
		* 设置帧完成事件的触发
		*/
		void SetFrameCompletedEvent();

	private:

		/*
		* ProcessFeedback线程
		*/
		void ProcessFeedbackThread();

		/*
		* UpdateResidencyMipMap线程
		*/
		void UpdateResidencyMipMap();

	private:
		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker*  mFrameTracker{ nullptr };
		std::unordered_map<std::string, std::unique_ptr<StreamTexture>>* mTextureStorage{ nullptr };
		DataUploader* mDataUploader{ nullptr };

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessFeedbackThread;

	};

}