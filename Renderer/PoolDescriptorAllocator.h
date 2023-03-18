#pragma once
#include "GHL/DescriptorHeap.h"
#include "Tools/Wrap.h"
#include "Tools/Pool.h"
#include "RingFrameTracker.h"
#include <optional>

namespace Renderer {

	using DescriptorHandleWrap = Tool::Wrap<GHL::DescriptorHandle>;

	/*
	* 池化的描述符分配器，负责描述符的创建与重用，不负责描述符的释放
	*/
	class PoolDescriptorAllocator {
	public:
		struct SlotUserData {
			std::optional<uint64_t> indexInHeap = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserData>;

		struct Deallocation {
			typename Pool::Slot* slot{ nullptr };
			D3D12_DESCRIPTOR_HEAP_TYPE heapType;
		};

	public:
		PoolDescriptorAllocator(const GHL::Device* device, RingFrameTracker* ringFrameTracker, std::vector<uint64_t> capacity);
		PoolDescriptorAllocator(const PoolDescriptorAllocator& other) = delete;
		PoolDescriptorAllocator(PoolDescriptorAllocator&& other) = default;
		PoolDescriptorAllocator& operator=(const PoolDescriptorAllocator& other) = delete;
		PoolDescriptorAllocator& operator=(PoolDescriptorAllocator&& other) = default;
		~PoolDescriptorAllocator() = default;

		DescriptorHandleWrap Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type);

		/*
		* Get方法
		*/
		inline const auto& GetCBSRUADescriptorHeap()  const { return mCBSRUADescriptorHeap; }
		inline const auto& GetSamplerDescriptorHeap() const { return mSamplerDescriptorHeap; }
		inline const auto& GetRTDescriptorHeap()      const { return mRTDescriptorHeap; }
		inline const auto& GetDSDescriptorHeap()      const { return mDSDescriptorHeap; }

	private:
		void CleanUpPendingDeallocation(uint8_t frameIndex);

	private:
		const GHL::Device*  mDevice{ nullptr };
		RingFrameTracker*   mFrameTracker{ nullptr };

		Pool mCBSRUADescriptorPool;
		GHL::DescriptorHeap mCBSRUADescriptorHeap;
		std::vector<std::unique_ptr<GHL::DescriptorHandle>> mCBSRUADescriptors;

		Pool mSamplerDescriptorPool;
		GHL::DescriptorHeap mSamplerDescriptorHeap;
		std::vector<std::unique_ptr<GHL::DescriptorHandle>> mSamplerDescriptors;

		Pool mRTDescriptorPool;
		GHL::DescriptorHeap mRTDescriptorHeap;
		std::vector<std::unique_ptr<GHL::DescriptorHandle>> mRTDescriptors;

		Pool mDSDescriptorPool;
		GHL::DescriptorHeap mDSDescriptorHeap;
		std::vector<std::unique_ptr<GHL::DescriptorHandle>> mDSDescriptors;

		std::vector<std::vector<Deallocation>> mPendingDeallocations;
	};

}