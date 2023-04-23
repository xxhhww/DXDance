#include "FileHandle.h"

#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	FileHandle::FileHandle(IDStorageFactory* dstorageFactory, const std::string& filepath) 
	: mFilepath(filepath) {
		dstorageFactory->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile));
	}

}