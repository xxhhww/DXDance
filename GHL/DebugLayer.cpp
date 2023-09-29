#include "DebugLayer.h"
#include "Tools/Assert.h"

namespace GHL {

	void EnableDebugLayer() {
#ifdef _DEBUG
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		HRASSERT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
#endif // _DEBUG
	}

}