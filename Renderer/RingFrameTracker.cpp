#include "RingFrameTracker.h"

namespace Renderer {

	RingFrameTracker::RingFrameTracker(size_t maxSize) : mMaxSize(maxSize) {}

	void RingFrameTracker::PushCurrentFrame(uint64_t expectedValue) {
        mFrameAttributes.emplace_back(expectedValue, Allocate(1u), 1u);
	}

	void RingFrameTracker::PopCompletedFrame(uint64_t completedValue) {
        // We can release all tails whose associated fence value is less 
        // than or equal to CompletedFenceValue
        while (!mFrameAttributes.empty() && mFrameAttributes.front().fenceValue <= completedValue) {
            const auto& oldestFrameTail = mFrameAttributes.front();

            mUsedSize -= oldestFrameTail.size;
            mHead = oldestFrameTail.frameIndex + oldestFrameTail.size;
            for (const auto& callBack : mCompletedCallBacks) {
                callBack(oldestFrameTail.frameIndex);
            }
            mFrameAttributes.pop_front();
        }
	}

    void RingFrameTracker::AddCompletedCallBack(const FrameCompletedCallBack& callBack) {
        mCompletedCallBacks.push_back(callBack);
    }

	bool RingFrameTracker::IsFull() const {
        return mUsedSize == mMaxSize;
	}

	size_t RingFrameTracker::Allocate(size_t size) {
		if (IsFull()) {
			return Invalid;
		}

        if (mTail >= mHead) {
            //
            //                     Head             Tail     MaxSize
            //                     |                |        |
            //  [                  xxxxxxxxxxxxxxxxx         ]
            //                                        
            if (mTail + size <= mMaxSize) {
                auto offset = mTail;
                mTail += size;
                mUsedSize += size;
                return offset;
            }
            else if (size <= mHead) {
                // Allocate from the beginning of the buffer
                size_t addSize = (mMaxSize - mTail) + size;
                mUsedSize += addSize;
                mTail = size;
                return 0;
            }
        }
        else if (mTail + size <= mHead) {
            //
            //       Tail          Head            
            //       |             |            
            //  [xxxx              xxxxxxxxxxxxxxxxxxxxxxxxxx]
            //
            auto offset = mTail;
            mTail += size;
            mUsedSize += size;
            return offset;
        }

        return Invalid;
	}
}