#include "Renderer/Misc.h"

namespace Renderer {

	void EnqueueDStorageRequest(GHL::DirectStorageQueue* dstorageQueue, void* src, uint32_t size, GHL::Resource* dst, uint64_t offset) {
		DSTORAGE_REQUEST request{};
		request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
		request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
		request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

		request.Source.Memory.Source = src;
		request.Source.Memory.Size = size;
		request.Destination.Buffer.Resource = dst->D3DResource();
		request.Destination.Buffer.Offset = offset;
		request.Destination.Buffer.Size = size;
		request.UncompressedSize = size;

		dstorageQueue->EnqueueRequest(&request);
	}

}