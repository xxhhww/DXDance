#pragma once
#include <memory>

namespace GHL {
	class Device;
}

namespace Renderer {

	class RingFrameTracker;
	class StreamTextureStorage;

	/*
	* 磁贴更新器，在一个渲染帧完成后，负责更新纹理使用的磁贴，并更新全局ResidencyMap(用于指示纹理在显存中保留的最细节的MipLevel)
	*/
	class TileUpdater {
	public:
		TileUpdater(const GHL::Device* device, RingFrameTracker* frameTracker);
		~TileUpdater() = default;

	private:
		/*
		* 帧完成后的回调函数
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

	private:
		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker*  mFrameTracker{ nullptr };

		std::unique_ptr<StreamTextureStorage> mTexureStorage;
	};

}