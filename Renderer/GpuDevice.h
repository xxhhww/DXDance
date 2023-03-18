#pragma once
#include "GHL/Device.h"
#include "GHL/AdapterContainer.h"
#include "GHL/CommandQueue.h"

#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "PoolCommandListAllocator.h"

namespace Renderer {

	/*
	* GPU�豸����GPU������еĳ����ʾ
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
		GHL::Device           device;           // ��ʾ���豸
		GHL::AdapterContainer adapterContainer; // �û���������������
		GHL::Adapter&         currentAdapter;   // ����ʹ�õ�������(Ĭ����������ܵ�)

		GHL::GraphicsQueue    graphicsQueue;    // ͼ������
		GHL::ComputeQueue     computeQueue;     // ��������
		GHL::CopyQueue        copyQueue;        // ��������

		GHL::Fence            renderFrameFence; // ��Ⱦ֡Χ��

		// ������һЩ����������Щ����������Դ�������ö��߶�����renderFrameFence��ֵ

		BuddyHeapAllocator        heapAllocator;        // �ѷ�����
		PoolCommandListAllocator  commandListAllocator; // �����б������
		PoolDescriptorAllocator   descriptorAllocator;  // ������������

	};

}