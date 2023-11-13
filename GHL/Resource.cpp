#include "Resource.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

namespace GHL {

	void Resource::SetDebugName(const std::string& name) {
		mName = name;
		HRASSERT(mD3DResource->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
	}

	const std::string& Resource::GetDebugName() {
		return mName;
	}

}