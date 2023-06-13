#include "CommandBuffer.h"
#include "RenderGraph.h"
#include "ShaderManger.h"
#include "LinearBufferAllocator.h"
#include "CommandSignatureManger.h"
#include "RenderGraphResourceStorage.h"

#include "GHL/CommandList.h"

namespace Renderer {

	CommandBuffer::CommandBuffer(GHL::CommandList* commandList, RenderContext* renderContext)
	: mCommandList(commandList)
	, mRenderContext(renderContext) {}

	void CommandBuffer::SetGraphicsRootSignature(const std::string& name) {
		auto* shaderManger = mRenderContext->shaderManger;
		ID3D12RootSignature* rootSignature = shaderManger->GetBaseD3DRootSignature();
		mCommandList->D3DCommandList()->SetGraphicsRootSignature(rootSignature);
	}

	void CommandBuffer::SetComputeRootSignature(const std::string& name) {
		auto* shaderManger = mRenderContext->shaderManger;
		ID3D12RootSignature* rootSignature = shaderManger->GetBaseD3DRootSignature();
		mCommandList->D3DCommandList()->SetComputeRootSignature(rootSignature);
	}

	void CommandBuffer::SetGraphicsPipelineState(const std::string& name) {
		auto* shaderManger = mRenderContext->shaderManger;
		ID3D12PipelineState* pipelineState = shaderManger->GetShader<GraphicsShader>(name)->GetD3DPipelineState();
		mCommandList->D3DCommandList()->SetPipelineState(pipelineState);
	}

	void CommandBuffer::SetComputePipelineState(const std::string& name) {
		auto* shaderManger = mRenderContext->shaderManger;
		ID3D12PipelineState* pipelineState = shaderManger->GetShader<ComputeShader>(name)->GetD3DPipelineState();
		mCommandList->D3DCommandList()->SetPipelineState(pipelineState);
	}

	void CommandBuffer::SetGraphicsRootCBV(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) {
		mCommandList->D3DCommandList()->SetGraphicsRootConstantBufferView(rootParamIndex, gpuAddress);
	}

	void CommandBuffer::SetComputeRootCBV(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) {
		mCommandList->D3DCommandList()->SetComputeRootConstantBufferView(rootParamIndex, gpuAddress);
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

	void CommandBuffer::ClearRenderTarget(Texture* rtTexture, float* clearColor, std::optional<GHL::Rect> rect) {
		auto& rtHandle = rtTexture->GetRTDescriptor()->GetCpuHandle();
		float defaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (rect != std::nullopt) {
			auto d3dObject = rect->D3DRect();
			mCommandList->D3DCommandList()->ClearRenderTargetView(rtHandle, (clearColor == nullptr) ? defaultClearColor : clearColor, 1u, &d3dObject);
		}
		else {
			mCommandList->D3DCommandList()->ClearRenderTargetView(rtHandle, (clearColor == nullptr) ? defaultClearColor : clearColor, 0u, nullptr);
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
		auto dyAlloc = mRenderContext->dynamicAllocator->Allocate(srcSize, 256u);
		memcpy(dyAlloc.cpuAddress, srcData, srcSize);
		mCommandList->D3DCommandList()->CopyBufferRegion(dstBuffer->D3DResource(), dstOffset, dyAlloc.backResource, dyAlloc.offset, srcSize);
	}

	void CommandBuffer::CopyBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, Buffer* srcBuffer, uint64_t srcOffset, uint64_t numBytes) {
		mCommandList->D3DCommandList()->CopyBufferRegion(dstBuffer->D3DResource(), dstOffset, srcBuffer->D3DResource(), srcOffset, numBytes);
	}

	void CommandBuffer::ClearCounterBuffer(Buffer* buffer, uint32_t value) {
		auto dyAlloc = mRenderContext->dynamicAllocator->Allocate(sizeof(uint32_t), 256u);
		memcpy(dyAlloc.cpuAddress, &value, sizeof(uint32_t));

		mCommandList->D3DCommandList()->CopyBufferRegion(buffer->GetCounterBuffer()->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, sizeof(uint32_t));
	}

	void CommandBuffer::CopyCounterBuffer(Buffer* dstBuffer, Buffer* srcBuffer) {
		this->CopyBufferRegion(dstBuffer->GetCounterBuffer(), 0u, srcBuffer->GetCounterBuffer(), 0u, sizeof(uint32_t));
	}

	GHL::ResourceBarrierBatch CommandBuffer::TransitionImmediately(Resource* resource, GHL::EResourceState newState, bool tryImplicitly) {
		auto* stateTracker = mRenderContext->resourceStateTracker;
		return stateTracker->TransitionImmediately(resource, newState, tryImplicitly);
	}
	
	GHL::ResourceBarrierBatch CommandBuffer::TransitionImmediately(Resource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly) {
		auto* stateTracker = mRenderContext->resourceStateTracker;
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


	void CommandBuffer::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
		mCommandList->D3DCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void CommandBuffer::ExecuteIndirect(const std::string& name, Buffer* argumentBuffer, uint32_t maxCommandCount) {
		auto* cmdSig = mRenderContext->commandSignatureManger->GetD3DCommandSignature(name);
		
		mCommandList->D3DCommandList()->ExecuteIndirect(cmdSig, maxCommandCount, argumentBuffer->D3DResource(), 0u, argumentBuffer->GetCounterBuffer()->D3DResource(), 0u);
	}

	ID3D12GraphicsCommandList4* CommandBuffer::D3DCommandList() const {
		return mCommandList->D3DCommandList();
	}

}