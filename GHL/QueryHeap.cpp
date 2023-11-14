#include "QueryHeap.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
	QueryHeap::QueryHeap(const Device* device, uint64_t size, EQueryHeapType queryHeapType)
	: mDevice(device)
	, mSize(size)
	, mQueryType(GetD3DQueryType(queryHeapType))
	, mQueryHeapType(GetD3DQueryHeapType(queryHeapType)) {

        mDesc.Count = size;
		mDesc.Type = mQueryHeapType;
		mDesc.NodeMask = device->GetNodeMask();
        HRASSERT(device->D3DDevice()->CreateQueryHeap(&mDesc, IID_PPV_ARGS(&mHeap)));
	}

	void QueryHeap::SetDebugName(const std::string& name) {
		mName = name;
		HRASSERT(mHeap->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
	}

	const std::string& QueryHeap::GetDebugName() {
		return mName;
	}
}