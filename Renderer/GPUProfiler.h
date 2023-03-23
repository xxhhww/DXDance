#pragma once
#include "Buffer.h"
#include "GHL/QueryHeap.h"
#include "RingFrameTracker.h"

#include <mutex>

namespace Renderer {

	class GPUProfiler {
	public:
		/*
		* ÿһ֡��Query����
		*/
		struct FrameQueryData {
		public:
			bool queryStarted  { false };
			bool queryFinished { false };

			std::string name{ "" };

			uint64_t frequency{ 0u };
		};

		/*
		* ���֡��Query����
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
		* ����GPU�����Ƶ��
		*/
		void SetPerQueueTimestampFrequencies(const std::vector<uint64_t> perQueueTimestampFrequencies);

		/*
		* ��¼������ʼ����
		*/
		uint32_t RecordProfileStart(ID3D12GraphicsCommandList* cmdList, const std::string& name, uint32_t queueIndex);
		
		/*
		* ��¼������������
		*/
		void RecordProfileEnd(ID3D12GraphicsCommandList* cmdList, uint32_t profileIndex);

		/*
		* ��¼�ض�����(ÿһ֡����)
		*/
		void RecordReadback(ID3D12GraphicsCommandList* cmdList);

		inline const auto& GetCompletedQueryDatas() const { return mCompletedQueryDatas; }

	private:
		/*
		* ��֡�����ʱ�Ļص�����
		*/
		void NewFramePushedCallback(uint8_t frameIndex);

		/*
		* ֡��ɺ�Ļص�����
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

		/*
		* ��ȡframeIndex��QueryHeap�е�ƫ����
		*/
		uint32_t GetHeapStartIndex(uint8_t frameIndex);

	private:
		static constexpr uint32_t smMaxProfilesPerFrame = 32u;       // ÿһ֡����ʹ�õ�Profile

		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };

		std::vector<uint64_t> mPerQueueTimestampFrequencies;         // ÿһ��GPU�����Ƶ��

		std::unique_ptr<GHL::QueryHeap> mQueryHeap;

		uint32_t mCurrFrameQueryDataCount{ 0u };                     // ��ǰ֡��QueryData����
		std::vector<std::vector<FrameQueryData>> mQueryDataPerFrame; // ÿһ֡��QueryData(ÿ֡����)
		std::vector<std::unique_ptr<Buffer>>     mReadbackBuffers;   // ÿһ֡��ReadbackBuffer(ÿ֡����)
		std::vector<CompletedQueryData> mCompletedQueryDatas;        // ���֡��QueryData(ÿһ֡��ɺ���¸�����)

		std::mutex mMutex;

	};

}