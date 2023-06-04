#include "pbh.h"
#include "Tools/Assert.h"
#include "Tools/VisitorHelper.h"

namespace GHL {

	D3D12_RESOURCE_STATES GetD3DResourceStates(EResourceState state) {
		D3D12_RESOURCE_STATES d3dState = D3D12_RESOURCE_STATE_COMMON;
		if (HasAllFlags(state, EResourceState::UnorderedAccess)) d3dState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (HasAllFlags(state, EResourceState::PixelShaderAccess)) d3dState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		if (HasAllFlags(state, EResourceState::NonPixelShaderAccess)) d3dState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (HasAllFlags(state, EResourceState::StreamOut)) d3dState |= D3D12_RESOURCE_STATE_STREAM_OUT;
		if (HasAllFlags(state, EResourceState::IndirectArgument)) d3dState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (HasAllFlags(state, EResourceState::CopyDestination)) d3dState |= D3D12_RESOURCE_STATE_COPY_DEST;
		if (HasAllFlags(state, EResourceState::CopySource)) d3dState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (HasAllFlags(state, EResourceState::GenericRead)) d3dState |= D3D12_RESOURCE_STATE_GENERIC_READ;
		if (HasAllFlags(state, EResourceState::RaytracingAccelerationStructure)) d3dState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (HasAllFlags(state, EResourceState::Predication)) d3dState |= D3D12_RESOURCE_STATE_PREDICATION;
		if (HasAllFlags(state, EResourceState::RenderTarget)) d3dState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (HasAllFlags(state, EResourceState::ResolveDestination)) d3dState |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
		if (HasAllFlags(state, EResourceState::ResolveSource)) d3dState |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		if (HasAllFlags(state, EResourceState::Present)) d3dState |= D3D12_RESOURCE_STATE_PRESENT;
		if (HasAllFlags(state, EResourceState::DepthRead)) d3dState |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (HasAllFlags(state, EResourceState::DepthWrite)) d3dState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (HasAllFlags(state, EResourceState::IndexBuffer)) d3dState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (HasAllFlags(state, EResourceState::VertexAndConstantBuffer)) d3dState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		return d3dState;
	}

	bool IsReadResourceState(EResourceState state) {
		EResourceState readStates =
			EResourceState::VertexAndConstantBuffer |
			EResourceState::IndexBuffer |
			EResourceState::NonPixelShaderAccess |
			EResourceState::PixelShaderAccess |
			EResourceState::IndirectArgument |
			EResourceState::CopySource |
			EResourceState::DepthRead;

		return HasAnyFlag(state, readStates);
	}

	bool IsRWResourceState(EResourceState state) {
		EResourceState rwStates =
			EResourceState::UnorderedAccess |
			EResourceState::DepthWrite;

		return HasAnyFlag(state, rwStates);
	}

	bool IsWriteResourceState(EResourceState state) {
		EResourceState writeStates =
			EResourceState::CopyDestination |
			EResourceState::RenderTarget |
			EResourceState::StreamOut;

		return HasAnyFlag(state, writeStates);
	}

	bool IsStatesSupportedOnQueue(EResourceState state, EGPUQueue queue) {
		switch (queue) {
		case GHL::EGPUQueue::Graphics:
			return true;	// Graphics 队列支持全部状态
		case GHL::EGPUQueue::Compute:
		{
			EResourceState allowedStates =
				EResourceState::Common |
				EResourceState::NonPixelShaderAccess |
				EResourceState::GenericRead |
				EResourceState::CopyDestination |
				EResourceState::CopySource |
				EResourceState::UnorderedAccess |
				EResourceState::RaytracingAccelerationStructure |
				EResourceState::VertexAndConstantBuffer;
			// 只要拥有除allowedStates以外的状态，就返回false
			return !HasAnyFlag(state, ~allowedStates);
		}
		case GHL::EGPUQueue::Copy:
		{
			EResourceState allowedStates =
				EResourceState::Common |
				EResourceState::CopyDestination |
				EResourceState::CopySource;
			// 只要拥有除allowedStates以外的状态，就返回false
			return !HasAnyFlag(state, ~allowedStates);
		}
		default:
			ASSERT_FORMAT(false, "Unsupported GPU Queue Type");
			return false;
		}
		return false;
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
			return D3D12_HEAP_TYPE_CUSTOM;
		}
	}

	D3D12_RESOURCE_DIMENSION GetD3DTextureDimension(ETextureDimension dimension) {
		switch (dimension) {
		case GHL::ETextureDimension::Texture1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case GHL::ETextureDimension::Texture2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case GHL::ETextureDimension::Texture3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default:
			ASSERT_FORMAT(false, "Unsupported Texture Dimension");
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		}
	}

	D3D12_QUERY_TYPE GetD3DQueryType(EQueryHeapType queryType) {
		switch (queryType) {
		case GHL::EQueryHeapType::Timestamp:
		case GHL::EQueryHeapType::CopyTimestamp:
			return D3D12_QUERY_TYPE_TIMESTAMP;
		default:
			ASSERT_FORMAT(false, "Unsupported Query Type");
			return D3D12_QUERY_TYPE_TIMESTAMP;
		}
	}

	D3D12_QUERY_HEAP_TYPE GetD3DQueryHeapType(EQueryHeapType queryType) {
		switch (queryType) {
		case GHL::EQueryHeapType::Timestamp:
			return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		case GHL::EQueryHeapType::CopyTimestamp:
			return D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP;
		default:
			ASSERT_FORMAT(false, "Unsupported Query Heap Type");
			return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		}
	}

	uint32_t GetFormatStride(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC4_UNORM:
			return 8u;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 16u;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12u;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 8u;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return 8u;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return 4u;
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return 2u;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
			return 1u;
		default:
			ASSERT_FORMAT(false, "Unsupport DXGI FORMAT");
			break;
		}
		return 16u;
	}

	DXGI_COLOR_SPACE_TYPE GetD3DColorSpace(ColorSpace space) {
		switch (space) {
		case ColorSpace::Rec709: return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		case ColorSpace::Rec2020: return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
		default: ASSERT_FORMAT(false, "Unsupported DXGI COLOR SPACE"); return {};
		}
	}

	ColorSpace GetColorSpace(DXGI_COLOR_SPACE_TYPE space) {
		switch (space) {
		case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709: return ColorSpace::Rec709;
		case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020: return ColorSpace::Rec2020;
		default:
			ASSERT_FORMAT(false, "Unsupported ColorSpace");
			return ColorSpace::Rec709;
		}
	}

	D3D12_CLEAR_VALUE GetD3DClearValue(const ClearValue& clearValue, DXGI_FORMAT format) {
		D3D12_CLEAR_VALUE d3dClearValue{};
		d3dClearValue.Format = format;

		std::visit(MakeVisitor(
			[&](const ColorClearValue& value) {
				d3dClearValue.Color[0] = value.x;
				d3dClearValue.Color[1] = value.y;
				d3dClearValue.Color[2] = value.z;
				d3dClearValue.Color[3] = value.w;
			},
			[&](const DepthStencilClearValue& value) {
				d3dClearValue.DepthStencil.Depth = value.depth;
				d3dClearValue.DepthStencil.Stencil = value.stencil;
			}
			), clearValue);

		return d3dClearValue;
	}
}