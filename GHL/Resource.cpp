#include "Resource.h"
#include "Tools/StrUtil.h"

namespace GHL {

	Resource::Resource(const Device* device)
	: mDevice(device) {}

	void Resource::SetDebugName(const std::string& name) {
		mResource->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
	}

}