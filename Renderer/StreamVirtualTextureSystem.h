#pragma once
#include "Renderer/StreamVirtualTexture.h"
#include "Renderer/TileUploader.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/RingFrameTracker.h"
#include "GHL/DirectStorageQueue.h"
#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

#include <memory>

namespace Renderer 
{

	class StreamVirtualTextureSystem {
	public:
		StreamVirtualTextureSystem(
			const GHL::Device* device,
			const GHL::Fence* mainFrameFence,
			GHL::DirectStorageFactory* dStorageFactory,
			GHL::DirectStorageQueue* dStorageFileCopyQueue,
			Renderer::PoolDescriptorAllocator* mainPoolDescriptorAllocator,
			Renderer::BuddyHeapAllocator* mainBuddyHeapAllocator,
			Renderer::RingFrameTracker* mainRingFrameTracker
		);

		~StreamVirtualTextureSystem();

		// called by stream virtual texture to update their minmipmap
		void SetResidencyChanged();

	private:
		// ��ʼ��ͼ�ζ���
		void InitializeGraphicsObject();

		// �����߳�
		void ProcessThread();

		// ����פ����Ϣ�߳�
		void UpdateResidencyThread();

	private:
		// Graphics Object
		const GHL::Device* mDevice{ nullptr };
		const GHL::Fence* mMainFrameFence{ nullptr };
		GHL::DirectStorageFactory* mDStorageFactory{ nullptr };
		GHL::DirectStorageQueue* mDStorageFileCopyQueue{ nullptr };

		Renderer::PoolDescriptorAllocator* mMainPoolDescriptorAllocator{ nullptr };
		Renderer::BuddyHeapAllocator* mMainBuddyHeapAllocator{ nullptr };
		Renderer::RingFrameTracker* mMainRingFrameTracker{ nullptr };

		std::unique_ptr<GHL::Fence> mPackedMipMappingFence;
		std::unique_ptr<GHL::Fence> mFileCopyFence;			// ��������ϴ�����
		std::unique_ptr<GHL::CommandQueue> mMappingQueue;
		std::unique_ptr<GHL::Fence> mMappingFence;			// ����Դ�ӳ�����

		// Tile Uploader
		std::unique_ptr<TileUploader> mTileUploader;

		// ��������
		std::vector<std::unique_ptr<StreamVirtualTexture>> mStreamVirtualTextures;

		// Thread Parameters
		bool mThreadRunning{ true };
		std::thread mProcessThread;
		std::thread mUpdateResidencyThread;
		HANDLE mResidencyChangedEvent{ nullptr };
	};

}