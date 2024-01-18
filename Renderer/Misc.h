#pragma once
#include "GHL/DirectStorageQueue.h"
#include "GHL/Resource.h"

namespace Renderer {

	void EnqueueDStorageRequest(GHL::DirectStorageQueue* dstorageQueue, void* src, uint32_t size, GHL::Resource* dst, uint64_t offset);

}