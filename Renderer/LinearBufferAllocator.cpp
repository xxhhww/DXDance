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
		// ������Ҫ��������ֽ�
		size_t alignedByteSize = Math::AlignUp(size, alignment);

		// ��LinearBufferΪ�ջ���LinearBuffer�Ŀռ䲻��ʱ������һ���µ�LinearBuffer
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
		// LinearBuffer��Ϊ�գ���������pendingDeallocations��
		if (mCurrLinearBuffer != nullptr) {
			mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(mCurrLinearBuffer);
			mCurrLinearBuffer = nullptr;
		}

		// ��LinearBufferΪ�ս��д���

		if (!mAvailableBuffers.empty()) {
			// ���ڿ����õ�LinearBuffer
			mCurrLinearBuffer = mAvailableBuffers.front();
			mCurrOffset = 0u;
			mAvailableBuffers.pop();
			return;
		}

		// �����ڿ����õ�LinearBuffer�������µ�LinearBuffer
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