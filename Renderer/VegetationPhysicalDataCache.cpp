#include "Renderer/VegetationPhysicalDataCache.h"

namespace Renderer {

	VegetationDataCache::VegetationDataCache(RenderEngine* renderEngine) 
	: mRenderEngine(renderEngine) {
	}

	// ���ò�ȺCahce�ĳ�ʼ������
	void VegetationDataCache::ConfigureGrassClusterCache(uint32_t grassClusterCacheCount, uint32_t maxGrassBladeCountPerCluster) {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		uint32_t nodeIndex = 0u;
		mGrassClusterCache = std::make_unique<GrassClusterCache>(grassClusterCacheCount);
		mGrassClusterCache->Foreach([&](GrassClusterCache::Node* node) {
			node->userData.bufferByteOffset = nodeIndex * sizeof(GrassBlade) * maxGrassBladeCountPerCluster;
			nodeIndex++;
		});

		BufferDesc _GrassBladePhysicalBuffer{};
		_GrassBladePhysicalBuffer.stride = sizeof(GrassBlade);
		_GrassBladePhysicalBuffer.size = grassClusterCacheCount * sizeof(GrassBlade) * maxGrassBladeCountPerCluster;
		_GrassBladePhysicalBuffer.usage = GHL::EResourceUsage::Default;
		_GrassBladePhysicalBuffer.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GrassBladePhysicalBuffer.initialState = GHL::EResourceState::Common;
		_GrassBladePhysicalBuffer.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::NonPixelShaderAccess;
		mGrassBladePhysicalBuffer = resourceAllocator->Allocate(device, _GrassBladePhysicalBuffer, descriptorAllocator, nullptr);
	}

	// ���ò�����Cache�ĳ�ʼ������
	void VegetationDataCache::ConfigureGrassMaskCache(uint32_t grassMaskCacheCount, uint32_t resolutionPerTile) {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

	}

	// ����GrassClusterCache(���ܼ���ʧ�ܣ�������ΪGrassClusterRect��Ӧ��Cache�Ѿ�������GrassClusterRectʹ����)
	VegetationDataCache::GrassClusterCache::Node* VegetationDataCache::ActivateGrassClusterCache(const Math::Vector4& targetGrassClusterRect) {
		// �������е�CacheNode
		GrassClusterCache::Node* targetNode = nullptr;
		mGrassClusterCache->Foreach([&](GrassClusterCache::Node* node) {
			if (node->userData.opGrassClusterRect == std::nullopt) return;

			if (*node->userData.opGrassClusterRect == targetGrassClusterRect) {
				targetNode == node;
			}
		});
		mGrassClusterCache->SetActive(targetNode);

		return targetNode;
	}

	// ��ȡ��ǰ���õ�GrassClusterCacheNode
	VegetationDataCache::GrassClusterCache::Node* VegetationDataCache::GetAvailableGrassClusterCache() {
		return mGrassClusterCache->GetHead();
	}

}