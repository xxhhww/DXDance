#include "pbh.h"
#include "Tools/Assert.h"

namespace GHL {


	D3D12_HEAP_TYPE GetD3DHeapType(EResourceUsage usage) {
		switch (usage)
		{
		case GHL::EResourceUsage::Upload:
			break;
		case GHL::EResourceUsage::ReadBack:
			break;
		case GHL::EResourceUsage::Default:
			break;
		default:
			break;
		}
	}
}