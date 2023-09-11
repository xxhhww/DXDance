#include "ResourceAllocator.h"

namespace Renderer {

	ResourceAllocator::ResourceAllocator(RingFrameTracker* ringFrameTracker)
	: mFrameTracker(ringFrameTracker) {
		mFrameTracker->AddFrameCompletedCallBack(
			[this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
				CleanUpPendingDeallocation(attribute.frameIndex);
			}
		);

		mPendingDeallocations.resize(mFrameTracker->GetMaxSize());
	}

	ResourceAllocator::~ResourceAllocator() {}
	
	TextureWrap ResourceAllocator::Allocate(
		const GHL::Device* device,
		const TextureDesc& textureDesc,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator) {
		auto* slot = mResourcePool.Allocate();
		if (!slot->userData.resourceIndex) {
			mResources.emplace_back(new Texture(device, ResourceFormat{ device, textureDesc }, descriptorAllocator, heapAllocator));
			slot->userData.resourceIndex = mResources.size() - 1u;
			Deallocation deallocation{ slot };
			return TextureWrap{
				static_cast<Texture*>(mResources.back().get()),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		size_t resourceIndex = *slot->userData.resourceIndex;
		mResources[resourceIndex] = std::move(std::make_unique<Texture>(device, ResourceFormat{device, textureDesc}, descriptorAllocator, heapAllocator));
		Deallocation deallocation{ slot };
		return TextureWrap{
			static_cast<Texture*>(mResources.at(resourceIndex).get()),
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	TextureWrap ResourceAllocator::Allocate(
		const GHL::Device* device,
		const TextureDesc& textureDesc,
		PoolDescriptorAllocator* descriptorAllocator,
		const GHL::Heap* heap,
		size_t heapOffset) {
		auto* slot = mResourcePool.Allocate();
		if (!slot->userData.resourceIndex) {
			mResources.emplace_back(new Texture(device, ResourceFormat{ device, textureDesc }, descriptorAllocator, heap, heapOffset));
			slot->userData.resourceIndex = mResources.size() - 1u;
			Deallocation deallocation{ slot };
			return TextureWrap{
				static_cast<Texture*>(mResources.back().get()),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		size_t resourceIndex = *slot->userData.resourceIndex;
		mResources[resourceIndex] = std::make_unique<Texture>(device, ResourceFormat{ device, textureDesc }, descriptorAllocator, heap, heapOffset);
		Deallocation deallocation{ slot };
		return TextureWrap{
			static_cast<Texture*>(mResources.at(resourceIndex).get()),
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	BufferWrap ResourceAllocator::Allocate(
		const GHL::Device* device,
		const BufferDesc& bufferDesc,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator) {
		auto* slot = mResourcePool.Allocate();
		if (!slot->userData.resourceIndex) {
			mResources.emplace_back(new Buffer(device, ResourceFormat{ device, bufferDesc }, descriptorAllocator, heapAllocator));
			slot->userData.resourceIndex = mResources.size() - 1u;
			Deallocation deallocation{ slot };
			return BufferWrap{
				static_cast<Buffer*>(mResources.back().get()),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		size_t resourceIndex = *slot->userData.resourceIndex;
		mResources[resourceIndex] = std::make_unique<Buffer>(device, ResourceFormat{ device, bufferDesc }, descriptorAllocator, heapAllocator);
		Deallocation deallocation{ slot };
		return BufferWrap{
			static_cast<Buffer*>(mResources.at(resourceIndex).get()),
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	BufferWrap ResourceAllocator::Allocate(
		const GHL::Device* device,
		const BufferDesc& bufferDesc,
		PoolDescriptorAllocator* descriptorAllocator,
		const GHL::Heap* heap,
		size_t heapOffset) {
		auto* slot = mResourcePool.Allocate();
		if (!slot->userData.resourceIndex) {
			mResources.emplace_back(new Buffer(device, ResourceFormat{ device, bufferDesc }, descriptorAllocator, heap, heapOffset));
			slot->userData.resourceIndex = mResources.size() - 1u;
			Deallocation deallocation{ slot };
			return BufferWrap{
				static_cast<Buffer*>(mResources.back().get()),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}
		size_t resourceIndex = *slot->userData.resourceIndex;
		mResources[resourceIndex] = std::make_unique<Buffer>(device, ResourceFormat{ device, bufferDesc }, descriptorAllocator, heap, heapOffset);
		Deallocation deallocation{ slot };
		return BufferWrap{
			static_cast<Buffer*>(mResources.at(resourceIndex).get()),
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	void ResourceAllocator::CleanUpPendingDeallocation(uint8_t frameIndex) {
		for (const auto& deallocation : mPendingDeallocations[frameIndex]) {
			auto* slot = deallocation.slot;
			mResources.at(*slot->userData.resourceIndex) = nullptr;
			mResourcePool.Deallocate(slot);
		}
		mPendingDeallocations[frameIndex].clear();
	}

}