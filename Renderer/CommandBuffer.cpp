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

		// CounterBuffer的状态转换消耗可以忽略不计，因此直接在CommandBuffer内部进行状态转换
		auto barrierBatch = this->TransitionImmediately(buffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
		this->FlushResourceBarrier(barrierBatch);

		mCommandList->D3DCommandList()->CopyBufferRegion(buffer->GetCounterBuffer()->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, sizeof(uint32_t));
	}

	void CommandBuffer::CopyCounterBuffer(Buffer* dstBuffer, Buffer* srcBuffer) {
		// CounterBuffer的状态转换消耗可以忽略不计，因此直接在CommandBuffer内部进行状态转换
		auto barrierBatch = this->TransitionImmediately(dstBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
		barrierBatch += this->TransitionImmediately(srcBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
		this->FlushResourceBarrier(barrierBatch);

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

	void CommandBuffer::ExecuteIndirect(const std::string& name, Buffer* argumentBuffer, uint32_t maxCommandCount) {
		auto* cmdSig = mRenderContext->commandSignatureManger->GetD3DCommandSignature(name);
		
		// CounterBuffer的状态转换消耗可以忽略不计，因此直接在CommandBuffer内部进行状态转换
		auto barrierBatch = this->TransitionImmediately(argumentBuffer->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
		this->FlushResourceBarrier(barrierBatch);
		
		mCommandList->D3DCommandList()->ExecuteIndirect(cmdSig, maxCommandCount, argumentBuffer->D3DResource(), 0u, argumentBuffer->GetCounterBuffer()->D3DResource(), 0u);
	}

	ID3D12GraphicsCommandList4* CommandBuffer::D3DCommandList() const {
		return mCommandList->D3DCommandList();
	}

}