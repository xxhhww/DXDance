#pragma once
// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#include <d3dcompiler.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <pix.h>
#include <dxcapi.h>
#include <DirectXCollision.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "Math/Vector.h"
#include "Tools/EnumUtil.h"
#include "d3dx12.h"

#include <unordered_set>
#include <optional>
#include <variant>

//linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace GHL {

	/*
	* 资源状态
	*/
	enum class EResourceState : uint64_t {
		Common                          = 0		,
		UnorderedAccess                 = 1 << 1,
		PixelShaderAccess               = 1 << 2,
		NonPixelShaderAccess            = 1 << 3,
		StreamOut                       = 1 << 4,
		IndirectArgument                = 1 << 5,
		CopyDestination                 = 1 << 6,
		CopySource                      = 1 << 7,
		GenericRead                     = 1 << 8,
		RaytracingAccelerationStructure = 1 << 9,
		Predication                     = 1 << 10,
		RenderTarget                    = 1 << 11,
		ResolveDestination              = 1 << 12,
		ResolveSource                   = 1 << 13,
		Present                         = 1 << 14,
		DepthRead                       = 1 << 15,
		DepthWrite                      = 1 << 16,
		VertexAndConstantBuffer         = 1 << 18,

		AnyShaderAccess = PixelShaderAccess | NonPixelShaderAccess
	};
	ENABLE_BITMASK_OPERATORS(EResourceState);

	D3D12_RESOURCE_STATES GetD3DResourceStates(EResourceState state);

	enum class EGPUQueue : uint8_t {
		Graphics = 0,
		Compute = 1,
		Copy = 2,
		Count = 3
	};

	bool IsStatesSupportedOnQueue(EResourceState state, EGPUQueue queue);

	/*
	* 资源类型
	*/
	enum class EResourceUsage : uint8_t {
		Upload,
		ReadBack,
		Default
	};

	D3D12_HEAP_TYPE GetD3DHeapType(EResourceUsage usage);

	/*
	* 纹理类型
	*/
	enum class ETextureDimension {
		Texture1D,
		Texture2D,
		Texture3D,
	};

	D3D12_RESOURCE_DIMENSION GetD3DTextureDimension(ETextureDimension dimension);

	/*
	* 纹理杂项标记
	*/
	enum class ETextureMiscFlag : uint32_t {
		None,
		CubeTexture = 1 << 0
	};
	ENABLE_BITMASK_OPERATORS(ETextureMiscFlag);

	/*
	* 资源绑定的视图类型
	*/
	enum class EResourceBindFlag : uint32_t {
		None = 0,
		ShaderResource  = 1 << 0,
		UnorderedAccess = 1 << 1,
		RenderTarget    = 1 << 2,
		DepthStencil    = 1 << 3
	};
	ENABLE_BITMASK_OPERATORS(EResourceBindFlag);

	/*
	* Buffer杂项标记
	*/
	enum class EBufferMiscFlag : uint32_t {
		None = 0,
		RawBuffer        = 1 << 0,
		StructuredBuffer = 1 << 1,
		ConstantBuffer   = 1 << 2,
		IndirectArgs     = 1 << 3,
		AccelerateStruct = 1 << 4
	};
	ENABLE_BITMASK_OPERATORS(EBufferMiscFlag);

	/*
	* QueryHeapType
	*/
	enum class EQueryHeapType : uint8_t {
		Timestamp,
		CopyTimestamp
	};

	D3D12_QUERY_TYPE GetD3DQueryType(EQueryHeapType queryType);

	D3D12_QUERY_HEAP_TYPE GetD3DQueryHeapType(EQueryHeapType queryType);

	/*
	* 获得类型的步幅
	*/
	uint32_t GetFormatStride(DXGI_FORMAT format);

	/*
	* 色域
	*/
	enum class ColorSpace : uint8_t
	{
		Rec709, Rec2020
	};
	DXGI_COLOR_SPACE_TYPE GetD3DColorSpace(ColorSpace space);
	ColorSpace GetColorSpace(DXGI_COLOR_SPACE_TYPE space);

	/*
	* Clear Color
	*/
	using ColorClearValue = Math::Vector4;

	struct DepthStencilClearValue {
		float depth;
		uint8_t stencil;
	};

	using ClearValue = std::variant<ColorClearValue, DepthStencilClearValue>;
	D3D12_CLEAR_VALUE GetD3DClearValue(const ClearValue& clearValue, DXGI_FORMAT format);

}