#pragma once
#include "pbh.h"
#include <DirectStorage/dstorage.h>

namespace GHL {

	class DirectStorageFactory {
	public:
		DirectStorageFactory();

		inline IDStorageFactory* GetDStorageFactory() const { return mDStorageFactory.Get(); }
	private:
		inline static const uint32_t mStagingBufferSizeMB = 512u;

		Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
	};

}