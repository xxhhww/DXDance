#pragma once
#include "GHL/pbh.h"
#include "GHL/DirectStorageFactory.h"

namespace GHL {

	class DirectStorageFile {
	public:
		DirectStorageFile(const DirectStorageFactory* dstorageFactory, const std::string&  filepath);
		inline ~DirectStorageFile() = default;

		inline auto* GetDStorageFile() const { return mDStorageFile.Get(); }

	private:
		Microsoft::WRL::ComPtr<IDStorageFile> mDStorageFile;
	};

}