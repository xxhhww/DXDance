#pragma once
#include <memory>

namespace GHL {
	class Device;
}

namespace Renderer {

	class RingFrameTracker;

	/*
	* ��������������һ����Ⱦ֡��ɺ󣬸����������ʹ�õĴ�����������ȫ��ResidencyMap(����ָʾ�������Դ��б�������ϸ�ڵ�MipLevel)
	*/
	class TileUpdater {
	public:
		TileUpdater(const GHL::Device* device, RingFrameTracker* frameTracker);
		~TileUpdater() = default;

	private:
		/*
		* ֡��ɺ�Ļص�����
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

	private:
		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker*  mFrameTracker{ nullptr };
	};

}