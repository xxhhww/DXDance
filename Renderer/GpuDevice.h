#pragma once
#include "GHL/Device.h"
#include "GHL/AdapterContainer.h"
#include "GHL/CommandQueue.h"

#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "PoolCommandListAllocator.h"

namespace Renderer {

	/*
	* GPU设备，是GPU在软件中的抽象表示
	*/
	class GpuDevice {
	public:
		GpuDevice();
		GpuDevice(const GpuDevice& other) = delete;
		GpuDevice(GpuDevice&& other) = default;
		GpuDevice& operator=(const GpuDevice& other) = delete;
		GpuDevice& operator=(GpuDevice&& other) = default;
		~GpuDevice() = default;

	public:
		GHL::Device           device;           // 显示器设备
		GHL::AdapterContainer adapterContainer; // 该机器的所有适配器
		GHL::Adapter&         currentAdapter;   // 程序使用的适配器(默认是最高性能的)

		GHL::GraphicsQueue    graphicsQueue;    // 图形引擎
		GHL::ComputeQueue     computeQueue;     // 计算引擎
		GHL::CopyQueue        copyQueue;        // 复制引擎

		GHL::Fence            renderFrameFence; // 渲染帧围栏

		// 以下是一些分配器，这些分配器对资源的重利用都高度依赖renderFrameFence的值

		BuddyHeapAllocator        heapAllocator;        // 堆分配器
		PoolCommandListAllocator  commandListAllocator; // 命令列表分配器
		PoolDescriptorAllocator   descriptorAllocator;  // 描述符分配器

	};

}