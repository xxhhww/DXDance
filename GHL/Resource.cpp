#include "Resource.h"
#include "Tools/StrUtil.h"

namespace GHL {

	void Resource::SetDebugName(const std::string& name) {
		mD3DResource->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
	}

}