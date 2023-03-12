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
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "Tools/EnumUtil.h"

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
		Common = 0,
		VertexAndConstantBuffer = 0x1,
		IndexBuffer = 0x2,
		RenderTarget = 0x4,
		UnorderedAccess = 0x8,
		DepthWrite = 0x10,
		DepthRead = 0x20,
		NonPixelShaderResource = 0x40,
		PixelShaderResource = 0x80,
		AllShaderResource = PixelShaderResource | NonPixelShaderResource,
		IndirectArgument = 0x100,
		CopyDest = 0x200,
		CopySource = 0x400,
		RaytracingAccelerationStructure = 0x800,
		GenericRead = VertexAndConstantBuffer | IndexBuffer | DepthRead | NonPixelShaderResource | PixelShaderResource | IndirectArgument | CopySource,
	};
	ENABLE_BITMASK_OPERATORS(EResourceState);

	D3D12_RESOURCE_STATES GetResourceStates(EResourceState state);

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
	* 资源的视图类型
	*/
	enum class EResourceBindFlag : uint32_t {
		None = 0,
		ConstantBuffer  = 1 << 0,
		ShaderResource  = 1 << 1,
		RenderTarget    = 1 << 2,
		DepthStencil    = 1 << 3,
		UnorderedAccess = 1 << 4
	};
	ENABLE_BITMASK_OPERATORS(EResourceBindFlag);

	/*
	* Buffer杂项标记
	*/
	enum class EBufferMiscFlag : uint32_t {
		None,
		ConstantBuffer   = 1 << 0,
		IndirectArgs     = 1 << 1,
		AccelerateStruct = 1 << 2
	};
	ENABLE_BITMASK_OPERATORS(EBufferMiscFlag);

	/*
	* Query类型
	*/
	enum class EQueryType : uint8_t {
		Timestamp,
		CopyTimestamp
	};

	D3D12_QUERY_HEAP_TYPE GetD3DQueryType(EQueryType queryType);
}