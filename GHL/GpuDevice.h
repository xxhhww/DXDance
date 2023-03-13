#pragma once
#include "DescriptorAllocator.h"
#include "SegregatedPoolHeapAllocator.h"
#include "CommandQueue.h"
#include "Fence.h"
#include "RootSignature.h"
#include "AdapterContainer.h"
#include "PoolCommandListAllocator.h"

namespace GHL {
	/*
	* 总的GPU抽象设备
	*/
	class GpuDevice {
	public:
		GpuDevice();
		~GpuDevice() = default;

	public:
		Device                      device;               // 显示设备
		AdapterContainer            adapterContainer;     // 适配器容器

		DescriptorAllocator         descriptorAllocator;  // 描述符分配器
		SegregatedPoolHeapAllocator heapAllocator;        // 堆分配器
		PoolCommandListAllocator    commandListAllocator; // 命令列表分配器

		GraphicsQueue               graphicsQueue;        // 图形引擎
		Fence                       graphicsFence;        // 图形引擎围栏

		ComputeQueue                computeQueue;         // 计算引擎
		Fence                       computeFence;         // 计算引擎围栏

		CopyQueue                   copyQueue;            // 复制引擎
		Fence                       copyFence;            // 复制引擎围栏

		RootSignature               rootSignature;        // 终极(究极)根签名
	};
}