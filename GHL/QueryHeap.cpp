#include "QueryHeap.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
	QueryHeap::QueryHeap(const Device* device, uint64_t size, EQueryType queryType)
	: mDevice(device)
	, mSize(size)
	, mType(GetD3DQueryType(queryType)) {

        mDesc.Count = size;
		mDesc.Type = mType;
		mDesc.NodeMask = device->GetNodeMask();
        HRASSERT(device->D3DDevice()->CreateQueryHeap(&mDesc, IID_PPV_ARGS(&mHeap)));
	}

	void QueryHeap::SetDebugName(const std::string& name) {
		mHeap->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
	}
}