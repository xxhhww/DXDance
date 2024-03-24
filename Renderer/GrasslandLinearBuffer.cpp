#include "Renderer/GrasslandLinearBuffer.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

namespace Renderer {

	GrasslandLinearBuffer::GrasslandLinearBuffer(TerrainRenderer* renderer, uint32_t tileCount, uint32_t bytesPerBalde, uint32_t bladesPerTile)
	: mRenderer(renderer)
	, mTileCount(tileCount)
	, mBytesPerBlade(bytesPerBalde)
	, mBladesPerTile(bladesPerTile)
	, mBytesPerTile(bytesPerBalde * bladesPerTile) {

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		BufferDesc _GrasslandLinearBufferDesc{};
		_GrasslandLinearBufferDesc.stride = bytesPerBalde;
		_GrasslandLinearBufferDesc.size = mBytesPerTile * mTileCount;
		_GrasslandLinearBufferDesc.usage = GHL::EResourceUsage::Default;
		_GrasslandLinearBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GrasslandLinearBufferDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::NonPixelShaderAccess;
		mLinearBuffer = resourceAllocator->Allocate(device, _GrasslandLinearBufferDesc, descriptorAllocator, nullptr);
		mLinearBuffer->SetDebugName("GrasslandLinearBuffer");

		renderGraph->ImportResource("GrasslandLinearBuffer", mLinearBuffer);
		resourceStateTracker->StartTracking(mLinearBuffer);
	}

}