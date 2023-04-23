#pragma once

#include <string>
#include <wrl.h>

#include "DirectStorage/dstorage.h"

namespace Renderer {

	class FileHandle {
	public:
		FileHandle(IDStorageFactory* dstorageFactory, const std::string& filepath);
		~FileHandle() = default;

		inline const auto& GetFilePath()     const { return mFilepath; }

		inline auto* GetDStorageFile() const { return mDStorageFile.Get(); }

	private:
		std::string mFilepath;
		Microsoft::WRL::ComPtr<IDStorageFile> mDStorageFile;
	};

}