#include "GHL/DirectStorageFile.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

namespace GHL {

	DirectStorageFile::DirectStorageFile(const DirectStorageFactory* dstorageFactory, const std::string& filepath) {
		HRASSERT(dstorageFactory->GetDStorageFactory()->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile)));
	}

}