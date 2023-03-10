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
	* 资源类型
	*/
	enum class EResourceUsage {
		Upload,
		ReadBack,
		Default
	};

	/*
	* 
	*/
	D3D12_HEAP_TYPE GetD3DHeapType(EResourceUsage usage);

}