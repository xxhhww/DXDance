#pragma once
#include "pbh.h"
#include "DescriptorHeap.h"

namespace GHL {

	/*
	* ÃèÊö·û·ÖÅäÆ÷
	*/
	class DescriptorAllocator {
	public:
		DescriptorAllocator(const Device* device, std::vector<uint64_t> capacity);
		~DescriptorAllocator() = default;

		[[nodiscard]] DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type);

		void Deallocate(DescriptorHandle& handle);

		inline const auto& GetCBVSRVUAVHeap() const { return mCBVSRVUAVDescriptorHeap; }

	private:
		const Device*  mDevice{ nullptr };
		DescriptorHeap mCBVSRVUAVDescriptorHeap;
		DescriptorHeap mSamplerDescriptorHeap;
		DescriptorHeap mRTVDescriptorHeap;
		DescriptorHeap mDSVDescriptorHeap;
	};

}