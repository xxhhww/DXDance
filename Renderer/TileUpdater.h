#pragma once
#include "StreamTexture.h"
#include <Windows.h>
#include <thread>

namespace GHL {
	class Device;
	class Fence;
}

namespace Renderer {

	class RingFrameTracker;
	class StreamTextureManger;
	class DataUploader;

	/*
	* ��������������һ����Ⱦ֡��ɺ󣬸����������ʹ�õĴ�����������ȫ��ResidencyMap(����ָʾ�������Դ��б�������ϸ�ڵ�MipLevel)
	*/
	class TileUpdater {
	public:
		TileUpdater(
			const GHL::Device* device, 
			GHL::Fence* renderFrameFence,
			RingFrameTracker* frameTracker, 
			std::unordered_map<std::string, std::unique_ptr<StreamTexture>>* textureStorage, 
			DataUploader* dataUploader);
		~TileUpdater();

	private:

		/*
		* ProcessFeedback�߳�
		*/
		void ProcessFeedbackThread();

		/*
		* UpdateResidencyMipMap�߳�
		*/
		void UpdateResidencyMipMap();

	private:
		const GHL::Device* mDevice{ nullptr };
		GHL::Fence* mRenderFrameFence{ nullptr };
		RingFrameTracker*  mFrameTracker{ nullptr };
		std::unordered_map<std::string, std::unique_ptr<StreamTexture>>* mTextureStorage{ nullptr };
		DataUploader* mDataUploader{ nullptr };

		bool mThreadRunning{ true };
		std::thread mProcessFeedbackThread;

	};

}