#include "GPUProfiler.h"

namespace Renderer {

	GPUProfiler::GPUProfiler(const GHL::Device* device, RingFrameTracker* frameTracker)
	: mDevice(device)
	, mFrameTracker(frameTracker) {

		mFrameTracker->AddNewFramePushedCallBack([this](const size_t& frameIndex) {
			NewFramePushedCallback(frameIndex);
		});

		mFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
			FrameCompletedCallback(frameIndex);
		});

		mQueryHeap = std::make_unique<GHL::QueryHeap>(mDevice, smMaxProfilesPerFrame * 2 * mFrameTracker->GetMaxSize(), GHL::EQueryHeapType::Timestamp);

		mQueryDataPerFrame.resize(mFrameTracker->GetMaxSize());
		for (size_t i = 0; i < mFrameTracker->GetMaxSize(); i++) {
			mQueryDataPerFrame[i].resize(smMaxProfilesPerFrame);
		}

		mReadbackBuffers.resize(mFrameTracker->GetMaxSize());

		// readbackBuffer描述
		BufferDesc readbackBufferDesc{};
		readbackBufferDesc.size = smMaxProfilesPerFrame * 2 * sizeof(uint64_t) * mFrameTracker->GetMaxSize();
		readbackBufferDesc.usage = GHL::EResourceUsage::ReadBack;
		readbackBufferDesc.initialState = GHL::EResourceState::CopyDestination;
		readbackBufferDesc.expectedState = readbackBufferDesc.initialState;

		// 为每一个帧创建readbackBuffer
		for (size_t i = 0; i < mFrameTracker->GetMaxSize(); i++) {
			mReadbackBuffers[i] = std::make_unique<Buffer>(mDevice, ResourceFormat{ mDevice, readbackBufferDesc }, nullptr, nullptr);
		}

		mCompletedQueryDatas.resize(smMaxProfilesPerFrame);
	}

	void GPUProfiler::SetPerQueueTimestampFrequencies(const std::vector<uint64_t> perQueueTimestampFrequencies) {
		mPerQueueTimestampFrequencies = perQueueTimestampFrequencies;
	}

	uint32_t GPUProfiler::RecordProfileStart(ID3D12GraphicsCommandList* cmdList, const std::string& name, uint32_t queueIndex) {
		uint8_t frameIndex = mFrameTracker->GetCurrFrameIndex();

		std::lock_guard lck(mMutex);

		uint32_t profileIndex = mCurrFrameQueryDataCount++;

		auto& queryData = mQueryDataPerFrame[frameIndex][profileIndex];

		ASSERT_FORMAT(queryData.queryStarted == false, "QueryData Must not be Started");
		ASSERT_FORMAT(queryData.queryFinished == false, "QueryData Must not be Finished");

		uint32_t heapIndex = profileIndex * 2u;

		cmdList->EndQuery(mQueryHeap->D3DQueryHeap(), mQueryHeap->GetQueryType(), heapIndex + GetHeapStartIndex(frameIndex));

		queryData.queryStarted = true;
		queryData.name = name;
		queryData.frequency = mPerQueueTimestampFrequencies[queueIndex];

		return profileIndex;
	}

	void GPUProfiler::RecordProfileEnd(ID3D12GraphicsCommandList* cmdList, uint32_t profileIndex) {
		uint8_t frameIndex = mFrameTracker->GetCurrFrameIndex();

		std::lock_guard lck(mMutex);

		auto& queryData = mQueryDataPerFrame[frameIndex][profileIndex];

		ASSERT_FORMAT(queryData.queryStarted == true, "QueryData Must be Started");
		ASSERT_FORMAT(queryData.queryFinished == false, "QueryData Must not be Finished");

		uint32_t heapIndex = profileIndex * 2u + 1u;

		cmdList->EndQuery(mQueryHeap->D3DQueryHeap(), mQueryHeap->GetQueryType(), heapIndex + GetHeapStartIndex(frameIndex));

		queryData.queryFinished = true;
	}

	void GPUProfiler::RecordReadback(ID3D12GraphicsCommandList* cmdList) {
		uint8_t frameIndex = mFrameTracker->GetCurrFrameIndex();

		auto& currReadbackBuffer = mReadbackBuffers[frameIndex];

		cmdList->ResolveQueryData(mQueryHeap->D3DQueryHeap(), mQueryHeap->GetQueryType(), GetHeapStartIndex(frameIndex), mCurrFrameQueryDataCount, currReadbackBuffer->D3DResource(), 0);
	}

	void GPUProfiler::NewFramePushedCallback(uint8_t frameIndex) {
		mCurrFrameQueryDataCount = 0u;
		mQueryDataPerFrame[frameIndex].clear();
	}

	void GPUProfiler::FrameCompletedCallback(uint8_t frameIndex) {
		mCompletedQueryDatas.clear();

		auto& queryDatas = mQueryDataPerFrame[frameIndex];
		auto& readbackBuffer = mReadbackBuffers[frameIndex];

		uint64_t* tickData = reinterpret_cast<uint64_t*>(readbackBuffer->Map());
		
		for (size_t profileIndex = 0; profileIndex < queryDatas.size(); profileIndex++) {
			auto& queryData = queryDatas[profileIndex];

			uint32_t startIndex = profileIndex * 2u;
			uint32_t finishIndex = profileIndex * 2u + 1u;

			uint64_t startTick = tickData[startIndex];
			uint64_t finishTick = tickData[finishIndex];

			auto& completedData = mCompletedQueryDatas[profileIndex];
			completedData.durationMs = (float)finishTick - startTick / queryData.frequency;
			completedData.name = queryData.name;

			queryData.queryStarted = false;
			queryData.queryFinished = false;
			queryData.name = "";
			queryData.frequency = 0u;
		}
	}

	uint32_t GPUProfiler::GetHeapStartIndex(uint8_t frameIndex) {
		return smMaxProfilesPerFrame * frameIndex;
	}

}