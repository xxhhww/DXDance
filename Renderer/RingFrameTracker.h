#pragma once
#include <deque>
#include <functional>

namespace Renderer {

	/*
	* 环型数据结构，用来跟踪帧(Frame)的状态，并在状态改变时进行对应的处理
	*/
	class RingFrameTracker {
	public:
		struct FrameAttribute {
		public:
			FrameAttribute(uint64_t fenceValue, size_t frameIndex, size_t size) : fenceValue(fenceValue), frameIndex(frameIndex), size(size) {}
			~FrameAttribute() = default;

			uint64_t fenceValue; // 该FrameAttribute所代表的渲染帧所期望达到的围栏值
			size_t   frameIndex; // 该FrameAttribute所代表的帧索引
			size_t   size;       // 该FrameAttribute的长度(可弃用)
		};

		using NewFramePushedCallBack = std::function<void(const size_t&)>; // (const size_t&)表示帧索引
		using FrameCompletedCallBack = std::function<void(const size_t&)>; // (const size_t&)表示帧索引

		static const size_t Invalid = static_cast<size_t>(-1);

	public:
		RingFrameTracker(size_t maxSize);
		~RingFrameTracker() = default;

		/*
		* 向双向队列中压入当前帧所对应的FrameAttribute
		* @Param expectedValue: 当前帧完成后所期望达到的围栏值
		*/
		void PushCurrentFrame(uint64_t expectedValue);

		/*
		* 释放已经完成的FrameAttribute，并调用回收函数
		* @Param completedValue: 已达到(已完成)的围栏值
		*/
		void PopCompletedFrame(uint64_t completedValue);

		/*
		* 设置新的渲染帧开始录制命令时调用回调函数
		*/
		void AddNewFramePushedCallBack(const NewFramePushedCallBack& callBack);

		/*
		* 设置渲染帧被GPU完成时调用的回调函数
		*/
		void AddFrameCompletedCallBack(const FrameCompletedCallBack& callBack);


		bool IsFull() const;
		
		/*
		* Get方法
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
		* 执行分配操作，私有，由PushCurrentFrame调用
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