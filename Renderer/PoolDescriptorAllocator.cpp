#include "PoolDescriptorAllocator.h"
#include "Tools/Assert.h"

namespace Renderer {

	PoolDescriptorAllocator::PoolDescriptorAllocator(const GHL::Device* device, RingFrameTracker* ringFrameTracker, std::vector<uint64_t> capacity)
	: mDevice(device)
	, mFrameTracker(ringFrameTracker)
	, mCBSRUADescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, capacity.at(0))
	, mSamplerDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, capacity.at(1))
	, mRTDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, capacity.at(2))
	, mDSDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, capacity.at(3)) {

		mFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
			CleanUpPendingDeallocation(frameIndex);
		});

		mPendingDeallocations.resize(mFrameTracker->GetMaxSize());
	}

	DescriptorHandleWrap PoolDescriptorAllocator::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type) {
		switch (type) {
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: {
			auto* slot = mCBSRUADescriptorPool.Allocate();
			GHL::DescriptorHandle* handlePtr = nullptr;

			if (!slot->userData.indexInHeap) {
				auto handle = mCBSRUADescriptorHeap.Allocate(mCBSRUADescriptors.size());
				mCBSRUADescriptors.emplace_back(new GHL::DescriptorHandle(handle));

				slot->userData.indexInHeap = mCBSRUADescriptors.size() - 1u;
				handlePtr = mCBSRUADescriptors.back().get();
			}

			Deallocation deallocation{ slot, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
			return DescriptorHandleWrap{
				handlePtr,
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}

		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER: {
			auto* slot = mSamplerDescriptorPool.Allocate();
			GHL::DescriptorHandle* handlePtr = nullptr;

			if (!slot->userData.indexInHeap) {
				auto handle = mSamplerDescriptorHeap.Allocate(mSamplerDescriptors.size());
				mSamplerDescriptors.emplace_back(new GHL::DescriptorHandle(handle));

				slot->userData.indexInHeap = mSamplerDescriptors.size() - 1u;
				handlePtr = mSamplerDescriptors.back().get();
			}

			Deallocation deallocation{ slot, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER };
			return DescriptorHandleWrap{
				handlePtr,
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV: {
			auto* slot = mRTDescriptorPool.Allocate();
			GHL::DescriptorHandle* handlePtr = nullptr;

			if (!slot->userData.indexInHeap) {
				auto handle = mRTDescriptorHeap.Allocate(mRTDescriptors.size());
				mRTDescriptors.emplace_back(new GHL::DescriptorHandle(handle));

				slot->userData.indexInHeap = mRTDescriptors.size() - 1u;
				handlePtr = mRTDescriptors.back().get();
			}

			Deallocation deallocation{ slot, D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
			return DescriptorHandleWrap{
				handlePtr,
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV: {
			auto* slot = mDSDescriptorPool.Allocate();
			GHL::DescriptorHandle* handlePtr = nullptr;

			if (!slot->userData.indexInHeap) {
				auto handle = mDSDescriptorHeap.Allocate(mDSDescriptors.size());
				mDSDescriptors.emplace_back(new GHL::DescriptorHandle(handle));

				slot->userData.indexInHeap = mDSDescriptors.size() - 1u;
				handlePtr = mDSDescriptors.back().get();
			}

			Deallocation deallocation{ slot, D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
			return DescriptorHandleWrap{
				handlePtr,
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		default:
			ASSERT_FORMAT(false, "Unsupport Descriptor Heap Type");
			return DescriptorHandleWrap{ nullptr, nullptr };
		}
	}

	void PoolDescriptorAllocator::CleanUpPendingDeallocation(uint8_t frameIndex) {
		for (const auto& deallocation : mPendingDeallocations[frameIndex]) {
			switch (deallocation.heapType) {
			case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
				mCBSRUADescriptorPool.Deallocate(deallocation.slot);
				break;
			case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				mSamplerDescriptorPool.Deallocate(deallocation.slot);
				break;
			case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
				mRTDescriptorPool.Deallocate(deallocation.slot);
				break;
			case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
				mDSDescriptorPool.Deallocate(deallocation.slot);
				break;
			default:
				ASSERT_FORMAT(false, "Unsupport Descriptor Heap Type");
				break;
			}
		}
		mPendingDeallocations[frameIndex].clear();
	}

}