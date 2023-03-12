#include "Heap.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"
#include "Math/Helper.h"

namespace GHL {
	Heap::Heap(const Device* device, size_t size, EResourceUsage usage)
	: mDevice(device)
	, mAlighnedSize(Math::AlignUp(size, mDevice->GetHeapAlignment()))
	, mUsage(usage) {
		
		mDesc.Flags = D3D12_HEAP_FLAG_NONE;
		mDesc.Alignment = mDevice->GetHeapAlignment();
		mDesc.SizeInBytes = mAlighnedSize;

		mDesc.Properties.CreationNodeMask = mDevice->GetNodeMask();
		mDesc.Properties.VisibleNodeMask = mDevice->GetNodeMask();
		mDesc.Properties.Type = GetD3DHeapType(mUsage);

		HRASSERT(mDevice->D3DDevice()->CreateHeap(&mDesc, IID_PPV_ARGS(&mHeap)));
	}

	void Heap::SetDebugName(const std::string& name) {
		mHeap->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
	}
}