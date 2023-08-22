#include "Mesh.h"
#include "Buffer.h"

#include "GHL/Fence.h"

namespace Renderer {

	Mesh::Mesh(const GHL::Device* device,
		const ResourceFormat& vbFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator)
	: mDevice(device)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		mVertexBuffer = std::make_unique<Buffer>(mDevice, vbFormat, mDescriptorAllocator, mHeapAllocator);
	}

	Mesh::Mesh(const GHL::Device* device,
		const ResourceFormat& vbFormat,
		const ResourceFormat& ibFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator) 
	: mDevice(device) 
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		mVertexBuffer = std::make_unique<Buffer>(mDevice, vbFormat, mDescriptorAllocator, mHeapAllocator);
		mIndexBuffer = std::make_unique<Buffer>(mDevice, ibFormat, mDescriptorAllocator, mHeapAllocator);
	}

	Mesh::~Mesh() {
	}

	void Mesh::LoadDataFromMemory(IDStorageQueue* copyDsQueue, GHL::Fence* copyFence, std::vector<Vertex>& vertices) {
		mVertexCount = vertices.size();

		DSTORAGE_REQUEST request = {};

		if (!vertices.empty()) {
			// upload vertices
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

			request.Source.Memory.Source = static_cast<void*>(vertices.data());
			request.Source.Memory.Size = vertices.size() * sizeof(Vertex);
			request.Destination.Buffer.Resource = mVertexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = vertices.size() * sizeof(Vertex);
			request.UncompressedSize = vertices.size() * sizeof(Vertex);
			copyDsQueue->EnqueueRequest(&request);
		}

		copyFence->IncrementExpectedValue();
		copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
		copyDsQueue->Submit();
		copyFence->Wait();
	}

	void Mesh::LoadDataFromMemory(IDStorageQueue* copyDsQueue, GHL::Fence* copyFence, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
		mVertexCount = vertices.size();
		mIndexCount = indices.size();
		
		DSTORAGE_REQUEST request = {};
		
		if (!vertices.empty()) {
			// upload vertices
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

			request.Source.Memory.Source = static_cast<void*>(vertices.data());
			request.Source.Memory.Size = vertices.size() * sizeof(Vertex);
			request.Destination.Buffer.Resource = mVertexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = vertices.size() * sizeof(Vertex);
			request.UncompressedSize = vertices.size() * sizeof(Vertex);
			copyDsQueue->EnqueueRequest(&request);
		}

		if (!indices.empty()) {
			// upload indices
			request.Source.Memory.Source = static_cast<void*>(indices.data());
			request.Source.Memory.Size = indices.size() * sizeof(uint32_t);
			request.Destination.Buffer.Resource = mIndexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = indices.size() * sizeof(uint32_t);
			request.UncompressedSize = indices.size() * sizeof(uint32_t);
			copyDsQueue->EnqueueRequest(&request);
		}

		copyFence->IncrementExpectedValue();
		copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
		copyDsQueue->Submit();
		copyFence->Wait();
	}

}