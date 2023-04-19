#pragma once
#include "RingFrameTracker.h"
#include "Buffer.h"

namespace Renderer {

	struct LinearAllocation {
	public:
		ID3D12Resource* backResource{ nullptr };                      
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_NULL }; // GPU��ַ
		size_t offset{ 0u }; // Allocation��gpuAddress�����backResource��gpuAddress��ƫ����
		size_t size{ 0u };   // ��С
		void* cpuAddress{ nullptr }; // CPU��ַ
	};

	/*
	* ���ԵĹ����ڴ���������������ڴ�Ĵ���������
	*/
	class LinearBufferAllocator {
	public:
		LinearBufferAllocator(const GHL::Device* device, RingFrameTracker* frameTracker);
		~LinearBufferAllocator() = default;

		LinearAllocation Allocate(size_t size, size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	private:
		/*
		* ����LinearBuffer
		*/
		void ScheduleLinearBuffer();

		void CleanUpPendingDeallocation(uint8_t frameIndex);

	private:
		size_t mStandardBufferSize{ 0x200000 };
		BufferDesc mStandardBufferDesc{};

		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };

		std::vector<std::unique_ptr<Buffer>> mBufferPool;
		std::queue<Buffer*> mAvailableBuffers;

		Buffer* mCurrLinearBuffer{ nullptr };
		size_t  mCurrOffset{ 0u };

		std::vector<std::vector<Buffer*>> mPendingDeallocations;
	};

}