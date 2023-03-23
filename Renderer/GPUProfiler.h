#pragma once
#include "Buffer.h"
#include "GHL/QueryHeap.h"
#include "RingFrameTracker.h"

#include <mutex>

namespace Renderer {

	class GPUProfiler {
	public:
		/*
		* 每一帧的Query数据
		*/
		struct FrameQueryData {
		public:
			bool queryStarted  { false };
			bool queryFinished { false };

			std::string name{ "" };

			uint64_t frequency{ 0u };
		};

		/*
		* 完成帧的Query数据
		*/
		struct CompletedQueryData {
		public:
			float durationMs{ 0u };

			std::string name{ "" };
		};

	public:
		GPUProfiler(const GHL::Device* device, RingFrameTracker* frameTracker);
		~GPUProfiler() = default;

		/*
		* 设置GPU引擎的频率
		*/
		void SetPerQueueTimestampFrequencies(const std::vector<uint64_t> perQueueTimestampFrequencies);

		/*
		* 记录分析开始命令
		*/
		uint32_t RecordProfileStart(ID3D12GraphicsCommandList* cmdList, const std::string& name, uint32_t queueIndex);
		
		/*
		* 记录分析结束命令
		*/
		void RecordProfileEnd(ID3D12GraphicsCommandList* cmdList, uint32_t profileIndex);

		/*
		* 记录回读命令(每一帧调用)
		*/
		void RecordReadback(ID3D12GraphicsCommandList* cmdList);

		inline const auto& GetCompletedQueryDatas() const { return mCompletedQueryDatas; }

	private:
		/*
		* 新帧入队列时的回调函数
		*/
		void NewFramePushedCallback(uint8_t frameIndex);

		/*
		* 帧完成后的回调函数
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

		/*
		* 获取frameIndex在QueryHeap中的偏移量
		*/
		uint32_t GetHeapStartIndex(uint8_t frameIndex);

	private:
		static constexpr uint32_t smMaxProfilesPerFrame = 32u;       // 每一帧最多可使用的Profile

		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };

		std::vector<uint64_t> mPerQueueTimestampFrequencies;         // 每一个GPU引擎的频率

		std::unique_ptr<GHL::QueryHeap> mQueryHeap;

		uint32_t mCurrFrameQueryDataCount{ 0u };                     // 当前帧的QueryData个数
		std::vector<std::vector<FrameQueryData>> mQueryDataPerFrame; // 每一帧的QueryData(每帧更新)
		std::vector<std::unique_ptr<Buffer>>     mReadbackBuffers;   // 每一帧的ReadbackBuffer(每帧更新)
		std::vector<CompletedQueryData> mCompletedQueryDatas;        // 完成帧的QueryData(每一帧完成后更新该数据)

		std::mutex mMutex;

	};

}