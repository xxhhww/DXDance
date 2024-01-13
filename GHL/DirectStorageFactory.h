#pragma once
#include "pbh.h"
#include <DirectStorage/dstorage.h>

namespace GHL {

	class DirectStorageFactory {
	public:
		DirectStorageFactory(size_t stagingBufferSizeMB = 512u);

		inline IDStorageFactory* GetDStorageFactory() const { return mDStorageFactory.Get(); }

	private:
		Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
	};

}