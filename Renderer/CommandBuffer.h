#pragma once
#include <d3d12.h>
#include <string>
#include "GHL/Viewport.h"
#include "GHL/Rect.h"
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

		void SetViewport(const GHL::Viewport& viewport);

		void SetScissorRect(const GHL::Rect& rect);

		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

		void SetVertexBuffer(uint32_t startSlot, Buffer* vBuffer);

		void SetIndexBuffer(Buffer* iBuffer);

		void SetRenderTarget(Texture* rtTexture);

		void SetRenderTarget(Texture* rtTexture, Texture* dsTexture);


		void ClearRenderTarget(Texture* rtTexture, float* color = nullptr, std::optional<GHL::Rect> rect = std::nullopt);

		void ClearDepth(Texture* dsTexture, float clearDepth, std::optional<GHL::Rect> rect = std::nullopt);

		void ClearStencil(Texture* dsTexture, uint8_t clearStencil, std::optional<GHL::Rect> rect = std::nullopt);

		void ClearDepthStencil(Texture* dsTexture, float clearDepth, uint8_t clearStencil, std::optional<GHL::Rect> rect = std::nullopt);

		void UploadBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, void* srcData, uint64_t srcSize);

		void CopyBufferRegion(Buffer* dstBuffer, uint64_t dstOffset, Buffer* srcBuffer, uint64_t srcOffset, uint64_t numBytes);

		void ClearCounterBuffer(Buffer* buffer, uint32_t value);

		void CopyCounterBuffer(Buffer* dstBuffer, Buffer* srcBuffer);	
		
		GHL::ResourceBarrierBatch TransitionImmediately(Resource* buffer, GHL::EResourceState newState, bool tryImplicitly = true);
		
		GHL::ResourceBarrierBatch TransitionImmediately(Resource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly = true);

		void FlushResourceBarrier(const GHL::ResourceBarrierBatch& barrierBatch);
		

		void ExecuteIndirect(const std::string& name, Buffer* argumentBuffer, uint32_t maxCommandCount);

		ID3D12GraphicsCommandList4* D3DCommandList() const;
		
	private:
		GHL::CommandList* mCommandList{ nullptr };
		RenderContext*    mRenderContext{ nullptr };
	};

}