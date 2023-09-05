#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/CommandSignatureManger.h"
#include "Renderer/RenderGraphResourceStorage.h"

#include "GHL/CommandList.h"

#include "Tools/StrUtil.h"

#include <pix_win.h>

namespace Renderer {

	CommandBuffer::CommandBuffer(GHL::CommandList* commandList, RenderContext* renderContext)
	: mCommandList(commandList)
	, mShaderManger(renderContext->shaderManger)
	, mResourceStateTracker(renderContext->resourceStateTracker)
	, mLinearBufferAllocator(renderContext->dynamicAllocator)
	, mCommandSignatureManger(renderContext->commandSignatureManger) {}

	CommandBuffer::CommandBuffer(
		GHL::CommandList* commandList,
		ShaderManger* shaderManger,
		ResourceStateTracker* resourceStateTracker,
		LinearBufferAllocator* dynamicAllocator,
		CommandSignatureManger* commandSignatureManger)
	: mCommandList(commandList)
	, mShaderManger(shaderManger)
	, mResourceStateTracker(resourceStateTracker)
	, mLinearBufferAllocator(dynamicAllocator)
	, mCommandSignatureManger(commandSignatureManger) {}

	void CommandBuffer::PIXBeginEvent(const std::string& name) {
		::PIXBeginEvent(mCommandList->D3DCommandList(), 0, Tool::StrUtil::UTF8ToWString(name).c_str());
	}

	void CommandBuffer::PIXEndEvent() {
		::PIXEndEvent(mCommandList->D3DCommandList());
	}

	void CommandBuffer::SetGraphicsRootSignature(const std::string& name) {
		auto* shaderManger = mShaderManger;
		ID3D12RootSignature* rootSignature = shaderManger->GetBaseD3DRootSignature();
		mCommandList->D3DCommandList()->SetGraphicsRootSignature(rootSignature);
	}

	void CommandBuffer::SetComputeRootSignature(const std::string& name) {
		auto* shaderManger = mShaderManger;
		ID3D12RootSignature* rootSignature = shaderManger->GetBaseD3DRootSignature();
		mCommandList->D3DCommandList()->SetComputeRootSignature(rootSignature);
	}

	void CommandBuffer::SetGraphicsPipelineState(const std::string& name) {
		auto* shaderManger = mShaderManger;
		ID3D12PipelineState* pipelineState = shaderManger->GetShader<GraphicsShader>(name)->GetD3DPipelineState();
		mCommandList->D3DCommandList()->SetPipelineState(pipelineState);
	}

	void CommandBuffer::SetComputePipelineState(const std::string& name) {
		auto* shaderManger = mShaderManger;
		ID3D12PipelineState* pipelineState = shaderManger->GetShader<ComputeShader>(name)->GetD3DPipelineState();
		mCommandList->D3DCommandList()->SetPipelineState(pipelineState);
	}

	void CommandBuffer::SetGraphicsRootCBV(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) {
		mCommandList->D3DCommandList()->SetGraphicsRootConstantBufferView(rootParamIndex, gpuAddress);
	}

	void CommandBuffer::SetGraphicsRootSRV(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) {
		mCommandList->D3DCommandList()->SetGraphicsRootShaderResourceView(rootParamIndex, gpuAddress);
	}

	void CommandBuffer::SetComputeRootCBV(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) {
		mCommandList->D3DCommandList()->SetComputeRootConstantBufferView(rootParamIndex, gpuAddress);
	}

	void CommandBuffer::SetComputeRootSRV(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) {
		mCommandList->D3DCommandList()->SetComputeRootShaderResourceView(rootParamIndex, gpuAddress);
	}

	void CommandBuffer::SetViewport(const GHL::Viewport& viewport) {
		auto d3dObject = viewport.D3DViewport();
		mCommandList->D3DCommandList()->RSSetViewports(1u, &d3dObject);
	}

	void CommandBuffer::SetScissorRect(const GHL::Rect& rect) {
		auto d3dObject = rect.D3DRect();
		mCommandList->D3DCommandList()->RSSetScissorRects(1u, &d3dObject);
	}

	void CommandBuffer::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) {
		mCommandList->D3DCommandList()->IASetPrimitiveTopology(topology);
	}

	void CommandBuffer::SetVertexBuffer(uint32_t startSlot, Buffer* vBuffer) {
		auto vbView = vBuffer->GetVBDescriptor();
		mCommandList->D3DCommandList()->IASetVertexBuffers(startSlot, 1u, &vbView);
	}

	void CommandBuffer::SetIndexBuffer(Buffer* iBuffer) {
		auto ibView = iBuffer->GetIBDescriptor();
		mCommandList->D3DCommandList()->IASetIndexBuffer(&ibView);
	}

	void CommandBuffer::SetRenderTarget(Texture* rtTexture) {
		auto& rtHandle = rtTexture->GetRTDescriptor()->GetCpuHandle();
		mCommandList->D3DCommandList()->OMSetRenderTargets(1u, &rtHandle, false, nullptr);
	}

	void CommandBuffer::SetRenderTarget(Texture* rtTexture, Texture* dsTexture) {
		auto& rtHandle = rtTexture->GetRTDescriptor()->GetCpuHandle();
		auto& dsHandle = dsTexture->GetDSDescriptor()->GetCpuHandle();
		mCommandList->D3DCommandList()->OMSetRenderTargets(1u, &rtHandle, false, &dsHandle);
	}

	void CommandBuffer::SetRenderTargets(std::vector<Texture*>&& rtTextures) {
		uint32_t numRTVs = rtTextures.size();
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtHandles(numRTVs);
		for (uint32_t i = 0; i < numRTVs; i++) {
			rtHandles[i] = rtTextures[i]->GetRTDescriptor()->GetCpuHandle();
		}
		mCommandList->D3DCommandList()->OMSetRenderTargets(numRTVs, rtHandles.data(), false, nullptr);
	}

	void CommandBuffer::SetRenderTargets(std::vector<Texture*>&& rtTextures, Texture* dsTexture) {
		uint32_t numRTVs = rtTextures.size();
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtHandles(numRTVs);
		for (uint32_t i = 0; i < numRTVs; i++) {
			rtHandles[i] = rtTextures[i]->GetRTDescriptor()->GetCpuHandle();
		}
		auto& dsHandle = dsTexture->GetDSDescriptor()->GetCpuHandle();
		mCommandList->D3DCommandList()->OMSetRenderTargets(numRTVs, rtHandles.data(), false, &dsHandle);
	}

	void CommandBuffer::ClearRenderTarget(Texture* rtTexture, std::optional<Math::Vector4> optClearColor, std::optional<GHL::Rect> optRect) {
		auto& rtHandle = rtTexture->GetRTDescriptor()->GetCpuHandle();
		Math::Vector4 clearColor = (optClearColor == std::nullopt) ?
			rtTexture->GetResourceFormat().GetColorClearValue() : (*optClearColor);

		float realClearColor[4] = {
			clearColor.x, clearColor.y, clearColor.z, clearColor.w 
		};

		if (optRect != std::nullopt) {
			auto d3dObject = optRect->D3DRect();
			mCommandList->D3DCommandList()->ClearRenderTargetView(rtHandle, realClearColor, 1u, &d3dObject);
		}
		else {
			mCommandList->D3DCommandList()->ClearRenderTargetView(rtHandle, realClearColor, 0u, nullptr);
		}
	}

	void CommandBuffer::ClearDepth(Texture* dsTexture, float clearDepth, std::optional<GHL::Rect> rect) {
		auto& dsHandle = dsTexture->GetDSDescriptor()->GetCpuHandle();
		if (rect != std::nullopt) {
			auto d3dObject = rect->D3DRect();
			mCommandList->D3DCommandList()->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0u, 1u, &d3dObject);
		}
		else {
			mCommandList->D3DCommandList()->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0u, 0u, nullptr);
		}
	}

	void CommandBuffer::ClearStencil(Texture* dsTexture, uint8_t clearStencil, std::optional<GHL::Rect> rect) {
		auto& dsHandle = dsTexture->GetDSDescriptor()->GetCpuHandle();
		if (rect != std::nullopt) {
			auto d3dObject = rect->D3DRect();
			mCommandList->D3DCommandList()->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_STENCIL, 0.0f, clearStencil, 1u, &d3dObject);
		}
		else {
			mCommandList->D3DCommandList()->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_STENCIL, 0.0f, clearStencil, 0u, nullptr);
		}
	}

	void CommandBuffer::ClearDepthStencil(Texture* dsTexture, float clearDepth, uint8_t clearStencil, std::optional<GHL::Rect> rect) {
		auto& dsHandle = dsTexture->GetDSDescriptor()->GetCpuHandle();
		if (rect != std::nullopt) {
			auto d3dObject = rect->D3DRect();
			mCommandList->D3DCommandList()->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clearDepth, clearStencil, 1u, &d3dObject);
		}
		else {
			mCommandList->D3DCommandList()->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clearDepth, clearStencil, 0u, nullptr);
		}
	}

	void CommandBuffer::UploadBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, void* srcData, uint64_t srcSize) {
		auto dyAlloc = mLinearBufferAllocator->Allocate(srcSize, 256u);
		memcpy(dyAlloc.cpuAddress, srcData, srcSize);
		mCommandList->D3DCommandList()->CopyBufferRegion(dstBuffer->D3DResource(), dstOffset, dyAlloc.backResource, dyAlloc.offset, srcSize);
	}

	void CommandBuffer::CopyBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, Buffer* srcBuffer, uint64_t srcOffset, uint64_t numBytes) {
		mCommandList->D3DCommandList()->CopyBufferRegion(dstBuffer->D3DResource(), dstOffset, srcBuffer->D3DResource(), srcOffset, numBytes);
	}

	void CommandBuffer::ClearBufferWithValue(Buffer* dstBuffer, uint32_t value) {
		auto dyAlloc = mLinearBufferAllocator->Allocate(sizeof(uint32_t), 256u);
		memcpy(dyAlloc.cpuAddress, &value, sizeof(uint32_t));

		mCommandList->D3DCommandList()->CopyBufferRegion(dstBuffer->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, sizeof(uint32_t));
	}

	void CommandBuffer::ClearCounterBuffer(Buffer* buffer, uint32_t value) {
		auto dyAlloc = mLinearBufferAllocator->Allocate(sizeof(uint32_t), 256u);
		memcpy(dyAlloc.cpuAddress, &value, sizeof(uint32_t));

		mCommandList->D3DCommandList()->CopyBufferRegion(buffer->GetCounterBuffer()->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, sizeof(uint32_t));
	}

	void CommandBuffer::CopyCounterBuffer(Buffer* dstBuffer, Buffer* srcBuffer) {
		this->CopyBufferRegion(dstBuffer->GetCounterBuffer(), 0u, srcBuffer->GetCounterBuffer(), 0u, sizeof(uint32_t));
	}

	void CommandBuffer::CopyResource(Resource* dstResource, Resource* srcResource) {
		mCommandList->D3DCommandList()->CopyResource(dstResource->D3DResource(), srcResource->D3DResource());
	}

	GHL::ResourceBarrierBatch CommandBuffer::TransitionImmediately(Resource* resource, GHL::EResourceState newState, bool tryImplicitly) {
		auto* stateTracker = mResourceStateTracker;
		return stateTracker->TransitionImmediately(resource, newState, tryImplicitly);
	}
	
	GHL::ResourceBarrierBatch CommandBuffer::TransitionImmediately(Resource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly) {
		auto* stateTracker = mResourceStateTracker;
		return stateTracker->TransitionImmediately(resource, subresourceIndex, newState, tryImplicitly);
	}


	void CommandBuffer::FlushResourceBarrier(const GHL::ResourceBarrierBatch& barrierBatch) {
		if (barrierBatch.Empty()) {
			return;
		}

		mCommandList->D3DCommandList()->ResourceBarrier(barrierBatch.Size(), barrierBatch.D3DBarriers());
	}

	void CommandBuffer::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
		mCommandList->D3DCommandList()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void CommandBuffer::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) {
		mCommandList->D3DCommandList()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startIndexLocation);
	}

	void CommandBuffer::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
		mCommandList->D3DCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void CommandBuffer::ExecuteIndirect(const std::string& name, Buffer* argumentBuffer, uint32_t maxCommandCount) {
		auto* cmdSig = mCommandSignatureManger->GetD3DCommandSignature(name);
		
		mCommandList->D3DCommandList()->ExecuteIndirect(cmdSig, maxCommandCount, argumentBuffer->D3DResource(), 0u, argumentBuffer->GetCounterBuffer()->D3DResource(), 0u);
	}

	ID3D12GraphicsCommandList4* CommandBuffer::D3DCommandList() const {
		return mCommandList->D3DCommandList();
	}

}