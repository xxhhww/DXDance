#include "pbh.h"
#include "Tools/Assert.h"

namespace GHL {

	D3D12_RESOURCE_STATES GetResourceStates(EResourceState state) {
		D3D12_RESOURCE_STATES api_state = D3D12_RESOURCE_STATE_COMMON;
		if (HasAnyFlag(state, EResourceState::VertexAndConstantBuffer)) api_state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (HasAnyFlag(state, EResourceState::IndexBuffer)) api_state |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (HasAnyFlag(state, EResourceState::RenderTarget)) api_state |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (HasAnyFlag(state, EResourceState::UnorderedAccess)) api_state |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (HasAnyFlag(state, EResourceState::DepthWrite)) api_state |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (HasAnyFlag(state, EResourceState::DepthRead)) api_state |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (HasAnyFlag(state, EResourceState::NonPixelShaderResource)) api_state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (HasAnyFlag(state, EResourceState::PixelShaderResource)) api_state |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		if (HasAnyFlag(state, EResourceState::IndirectArgument)) api_state |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (HasAnyFlag(state, EResourceState::CopyDest)) api_state |= D3D12_RESOURCE_STATE_COPY_DEST;
		if (HasAnyFlag(state, EResourceState::CopySource)) api_state |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (HasAnyFlag(state, EResourceState::RaytracingAccelerationStructure)) api_state |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		return api_state;
	}

	D3D12_HEAP_TYPE GetD3DHeapType(EResourceUsage usage) {
		switch (usage) {
		case GHL::EResourceUsage::Upload:
			return D3D12_HEAP_TYPE_UPLOAD;
		case GHL::EResourceUsage::ReadBack:
			return D3D12_HEAP_TYPE_READBACK;
		case GHL::EResourceUsage::Default:
			return D3D12_HEAP_TYPE_DEFAULT;
		default:
			ASSERT_FORMAT(false, "Unsupported Heap Type");
		}
		return D3D12_HEAP_TYPE_CUSTOM;
	}

	D3D12_QUERY_HEAP_TYPE GetD3DQueryType(EQueryType queryType) {
		switch (queryType) {
		case GHL::EQueryType::Timestamp:
			return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		case GHL::EQueryType::CopyTimestamp:
			return D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP;
		default:
			ASSERT_FORMAT(false, "Unsupported Heap Type");
		}
		return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	}
}