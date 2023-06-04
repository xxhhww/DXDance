#pragma once
#include <d3d12.h>
#include <string>
#include "GHL/ResourceBarrierBatch.h"

namespace GHL {
	class CommandList;
}

namespace Renderer {

	struct RenderContext;
	class Resource;
	class Buffer;
	class Texture;

	class CommandBuffer {
	public:
		CommandBuffer(GHL::CommandList* commandList, RenderContext* renderContext);
		~CommandBuffer() = default;

		void SetGraphicsRootSignature(const std::string& name = "");

		void SetComputeRootSignature(const std::string& name = "");

		void SetGraphicsPipelineState(const std::string& name);

		void SetComputePipelineState(const std::string& name);

		void UploadBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, void* srcData, uint64_t srcSize);

		void CopyBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, Buffer* srcBuffer, uint64_t srcOffset, uint64_t numBytes);

		void ClearCounterBuffer(Buffer* buffer, uint32_t value);

		void CopyCounterBuffer(Buffer* dstBuffer, Buffer* srcBuffer);

		GHL::ResourceBarrierBatch TransitionImmediately(Resource* buffer, GHL::EResourceState newState, bool tryImplicitly = true);
		
		GHL::ResourceBarrierBatch TransitionImmediately(Resource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly = true);

		void FlushResourceBarrier(const GHL::ResourceBarrierBatch& barrierBatch);
		
		/*
		*   _In_  ID3D12CommandSignature *pCommandSignature,
            _In_  UINT MaxCommandCount,
            _In_  ID3D12Resource *pArgumentBuffer,
            _In_  UINT64 ArgumentBufferOffset,
            _In_opt_  ID3D12Resource *pCountBuffer,
            _In_  UINT64 CountBufferOffset
		*/
		void ExecuteIndirect(const std::string& name, Buffer* argumentBuffer, uint32_t maxCommandCount);

		ID3D12GraphicsCommandList4* D3DCommandList() const;
		
	private:
		GHL::CommandList* mCommandList{ nullptr };
		RenderContext*    mRenderContext{ nullptr };
	};

}