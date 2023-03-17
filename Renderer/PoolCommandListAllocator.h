#pragma once
#include "GHL/CommandList.h"
#include "GHL/Device.h"
#include <optional>

namespace Renderer {

	class PoolCommandListAllocator {
	public:

	public:
		PoolCommandListAllocator(const GHL::Device* device);
		PoolCommandListAllocator(const PoolCommandListAllocator& other) = delete;
		PoolCommandListAllocator(PoolCommandListAllocator&& other) = default;
		PoolCommandListAllocator& operator=(const PoolCommandListAllocator& other) = delete;
		PoolCommandListAllocator& operator=(PoolCommandListAllocator&& other) = default;
		~PoolCommandListAllocator() = default;

	private:
		const GHL::Device* mDevice{ nullptr };


	};

}