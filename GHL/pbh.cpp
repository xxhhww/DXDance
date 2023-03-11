#include "pbh.h"
#include "Tools/Assert.h"

namespace GHL {


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