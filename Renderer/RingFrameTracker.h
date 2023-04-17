#pragma once
#include <deque>
#include <functional>

namespace Renderer {

	/*
	* �������ݽṹ����������֡(Frame)��״̬������״̬�ı�ʱ���ж�Ӧ�Ĵ���
	*/
	class RingFrameTracker {
	public:
		struct FrameAttribute {
		public:
			FrameAttribute(uint64_t fenceValue, size_t frameIndex, size_t size) : fenceValue(fenceValue), frameIndex(frameIndex), size(size) {}
			~FrameAttribute() = default;

			uint64_t fenceValue; // ��FrameAttribute���������Ⱦ֡�������ﵽ��Χ��ֵ
			size_t   frameIndex; // ��FrameAttribute�������֡����
			size_t   size;       // ��FrameAttribute�ĳ���(������)
		};

		using NewFramePushedCallBack = std::function<void(const size_t&)>; // (const size_t&)��ʾ֡����
		using FrameCompletedCallBack = std::function<void(const size_t&)>; // (const size_t&)��ʾ֡����

		static const size_t Invalid = static_cast<size_t>(-1);

	public:
		RingFrameTracker(size_t maxSize);
		~RingFrameTracker() = default;

		/*
		* ��˫�������ѹ�뵱ǰ֡����Ӧ��FrameAttribute
		* @Param expectedValue: ��ǰ֡��ɺ��������ﵽ��Χ��ֵ
		*/
		void PushCurrentFrame(uint64_t expectedValue);

		/*
		* �ͷ��Ѿ���ɵ�FrameAttribute�������û��պ���
		* @Param completedValue: �Ѵﵽ(�����)��Χ��ֵ
		*/
		void PopCompletedFrame(uint64_t completedValue);

		/*
		* �����µ���Ⱦ֡��ʼ¼������ʱ���ûص�����
		*/
		void AddNewFramePushedCallBack(const NewFramePushedCallBack& callBack);

		/*
		* ������Ⱦ֡��GPU���ʱ���õĻص�����
		*/
		void AddFrameCompletedCallBack(const FrameCompletedCallBack& callBack);


		bool IsFull() const;
		
		/*
		* Get����
		*/
		inline const auto& GetHead()               const { return mHead; }
		inline const auto& GetTail()               const { return mTail; }
		inline const auto& GetMaxSize()            const { return mMaxSize; }
		inline const auto& GetUsedSize()           const { return mFrameAttributes.size(); }
		inline const auto& IsFirstFrame()          const { return mFirstFrame; }
		inline const auto& GetCurrFrameIndex()     const { return mFrameAttributes.back().frameIndex; }
		inline const auto& GetCurrFrameAttribute() const { return mFrameAttributes.back(); }

	private:
		/*
		* ִ�з��������˽�У���PushCurrentFrame����
		*/
		size_t Allocate(size_t size = 1u);

	private:
		std::deque<FrameAttribute> mFrameAttributes;
		size_t mHead{ 0u };
		size_t mTail{ 0u };
		size_t mMaxSize{ 0u };
		size_t mUsedSize{ 0u };
		bool mFirstFrame{ true };
		bool mRequireFlip{ false };
		std::vector<NewFramePushedCallBack> mNewFramePushedCallBacks;
		std::vector<FrameCompletedCallBack> mCompletedCallBacks;
	};

}