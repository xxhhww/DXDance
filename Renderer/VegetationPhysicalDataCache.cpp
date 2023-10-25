#include "Renderer/VegetationPhysicalDataCache.h"
#include "Renderer/FixedTextureHelper.h"

namespace Renderer {

	VegetationDataCache::VegetationDataCache(RenderEngine* renderEngine) 
	: mRenderEngine(renderEngine) {
	}

	// 配置草群Cahce的初始化参数
	void VegetationDataCache::ConfigureGrassClusterCache(uint32_t grassClusterCacheCount, uint32_t maxGrassBladeCountPerCluster) {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		uint32_t nodeIndex = 0u;
		mGrassClusterCache = std::make_unique<GrassClusterCache>(grassClusterCacheCount);
		mGrassClusterCache->Foreach([&](GrassClusterCache::Node* node) {
			node->userData.grassBladeBufferIndex = nodeIndex  * maxGrassBladeCountPerCluster;
			nodeIndex++;
		});

		BufferDesc _GrassBladeBufferDesc{};
		_GrassBladeBufferDesc.stride = sizeof(GrassBlade);
		_GrassBladeBufferDesc.size = grassClusterCacheCount * sizeof(GrassBlade) * maxGrassBladeCountPerCluster;
		_GrassBladeBufferDesc.usage = GHL::EResourceUsage::Default;
		_GrassBladeBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GrassBladeBufferDesc.initialState = GHL::EResourceState::Common;
		_GrassBladeBufferDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::NonPixelShaderAccess;
		mGrassBladeBuffer = resourceAllocator->Allocate(device, _GrassBladeBufferDesc, descriptorAllocator, nullptr);
		renderGraph->ImportResource("BakedGrassBladeBuffer", mGrassBladeBuffer);
		resourceStateTracker->StartTracking(mGrassBladeBuffer);
	}

	// 配置草遮罩Cache的初始化参数
	void VegetationDataCache::ConfigureGrassMaskCache(const std::string& pathname, uint32_t grassMaskCacheCount, uint32_t resolutionPerTile) {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

		// 创建GrassLayerMask
		mGrassLayerMask = FixedTextureHelper::LoadFromFile(device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence, pathname);
		renderGraph->ImportResource("GrassLayerMask", mGrassLayerMask);
		resourceStateTracker->StartTracking(mGrassLayerMask);
	}

	// 激活GrassClusterCache(可能激活失败，这是因为GrassClusterRect对应的Cache已经被其他GrassClusterRect使用了)
	VegetationDataCache::GrassClusterCache::Node* VegetationDataCache::ActivateGrassClusterCache(const Math::Vector4& targetGrassClusterRect) {
		// 遍历所有的CacheNode
		GrassClusterCache::Node* targetNode = nullptr;
		mGrassClusterCache->Foreach([&](GrassClusterCache::Node* node) {
			if (node->userData.opGrassClusterRect == std::nullopt) return;

			if (*node->userData.opGrassClusterRect == targetGrassClusterRect) {
				targetNode == node;
			}
		});

		if (targetNode != nullptr) {
			mGrassClusterCache->SetActive(targetNode);
		}

		return targetNode;
	}

	// 获取当前可用的GrassClusterCacheNode
	VegetationDataCache::GrassClusterCache::Node* VegetationDataCache::GetAvailableGrassClusterCache() {
		GrassClusterCache::Node* targetNode = mGrassClusterCache->GetHead();
		mGrassClusterCache->SetActive(targetNode);
		return targetNode;
	}

}