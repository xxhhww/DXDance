#include "LinearBufferAllocator.h"
#include "Math/Helper.h"

namespace Renderer {

	LinearBufferAllocator::LinearBufferAllocator(const GHL::Device* device, RingFrameTracker* frameTracker) 
	: mDevice(device)
	, mFrameTracker(frameTracker) {
		mStandardBufferDesc.size = mStandardBufferSize;
		mStandardBufferDesc.stride = 1u;
		mStandardBufferDesc.usage = GHL::EResourceUsage::Default;
		mStandardBufferDesc.initalState = GHL::EResourceState::AnyShaderAccess | GHL::EResourceState::Common | GHL::EResourceState::GenericRead;
		mStandardBufferDesc.expectedState = mStandardBufferDesc.initalState;

		mFrameTracker->AddCompletedCallBack([this](const size_t& frameIndex) {
			CleanUpPendingDeallocation(frameIndex);
		});

		mPendingDeallocations.resize(mFrameTracker->GetMaxSize());
	}

	LinearAllocation LinearBufferAllocator::Allocate(size_t size, size_t alignment) {
		// 计算需要分配多少字节
		size_t alignedByteSize = Math::AlignUp(size, alignment);

		// 当LinearBuffer为空或者LinearBuffer的空间不足时，分配一个新的LinearBuffer
		if (mCurrLinearBuffer == nullptr || mCurrOffset + alignedByteSize > mStandardBufferSize) {
			ScheduleLinearBuffer();
		}

		LinearAllocation allocation{};
		allocation.backResource = mCurrLinearBuffer->D3DResource();
		allocation.gpuAddress = mCurrLinearBuffer->GetGpuAddress() + mCurrOffset;
		allocation.cpuAddress = mCurrLinearBuffer->Map() + mCurrOffset;
		allocation.offset = mCurrOffset;
		allocation.size = alignedByteSize;

		mCurrOffset += alignedByteSize;

		return allocation;
	}

	void LinearBufferAllocator::ScheduleLinearBuffer() {
		// LinearBuffer不为空，将其塞入pendingDeallocations中
		if (mCurrLinearBuffer != nullptr) {
			mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(mCurrLinearBuffer);
			mCurrLinearBuffer = nullptr;
		}

		// 以LinearBuffer为空进行处理

		if (!mAvailableBuffers.empty()) {
			// 存在可重用的LinearBuffer
			mCurrLinearBuffer = mAvailableBuffers.front();
			mCurrOffset = 0u;
			mAvailableBuffers.pop();
			return;
		}

		// 不存在可重用的LinearBuffer，创建新的LinearBuffer
		mBufferPool.emplace_back(std::make_unique<Buffer>(mDevice, mStandardBufferDesc, nullptr, nullptr));
		mCurrLinearBuffer = mBufferPool.back().get();
		mCurrOffset = 0u;
	}

	void LinearBufferAllocator::CleanUpPendingDeallocation(uint8_t frameIndex) {
		for (const auto& deallocation : mPendingDeallocations[frameIndex]) {
			mAvailableBuffers.push(deallocation);
		}
		mPendingDeallocations[frameIndex].clear();

	}

}