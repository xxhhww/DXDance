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
	* �ܵ�GPU�����豸
	*/
	class GpuDevice {
	public:
		GpuDevice();
		~GpuDevice() = default;

	public:
		Device                      device;               // ��ʾ�豸
		AdapterContainer            adapterContainer;     // ����������

		DescriptorAllocator         descriptorAllocator;  // ������������
		SegregatedPoolHeapAllocator heapAllocator;        // �ѷ�����
		PoolCommandListAllocator    commandListAllocator; // �����б������

		GraphicsQueue               graphicsQueue;        // ͼ������
		Fence                       graphicsFence;        // ͼ������Χ��

		ComputeQueue                computeQueue;         // ��������
		Fence                       computeFence;         // ��������Χ��

		CopyQueue                   copyQueue;            // ��������
		Fence                       copyFence;            // ��������Χ��

		RootSignature               rootSignature;        // �ռ�(����)��ǩ��
	};
}