#pragma once
#include "ResourceFormat.h"
#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"

#include "Math/Vector.h"

#include <DirectStorage/dstorage.h>

namespace GHL {
	class Fence;
}

namespace Renderer {

	class Buffer;

	struct Vertex {
	public:
		Math::Vector3 position;
		Math::Vector2 uv;
		Math::Vector3 normal;
		Math::Vector3 tangent;
		Math::Vector3 bitangent;
		Math::Vector4 color;

		Math::Vector4 userData;		// 用户自定义数据
	public:
		Vertex() 
		: position(0.0f, 0.0f, 0.0f)
		, uv(0.0f, 0.0f)
		, normal(0.0f, 0.0f, 0.0f)
		, tangent(0.0f, 0.0f, 0.0f)
		, bitangent(0.0f, 0.0f, 0.0f)
		, color(0.0f, 0.0f, 0.0f, 0.0f) {}
		
		Vertex(
			const Math::Vector3& p,
			const Math::Vector2& u,
			const Math::Vector3& n,
			const Math::Vector3& t,
			const Math::Vector3& b,
			const Math::Vector4& c)
		: position(p)
		, uv(uv)
		, normal(n)
		, tangent(t) 
		, bitangent(b) 
		, color(c) {}
		
		Vertex(
			float px, float py, float pz,
			float u, float v,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float bx, float by, float bz,
			float cx, float cy, float cz, float cw)
		: position(px, py, pz)
		, uv(u, v)
		, normal(nx, ny, nz)
		, tangent(tx, ty, tz)
		, bitangent(bx, by, bz)
		, color(cx, cy, cz, cw) {}

	};


	class Mesh {
	public:
		Mesh(const GHL::Device* device,
			const ResourceFormat& vbFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);

		Mesh(const GHL::Device* device,
			const ResourceFormat& vbFormat,
			const ResourceFormat& ibFormat,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator);
		~Mesh();

		void LoadDataFromMemory(IDStorageQueue* copyDsQueue, GHL::Fence* copyFence, std::vector<Vertex>& vertices);
		
		void LoadDataFromMemory(IDStorageQueue* copyDsQueue, GHL::Fence* copyFence, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

		inline const auto& GetVertexCount() const { return mVertexCount; }
		inline const auto& GetIndexCount()  const { return mIndexCount;  }

		inline auto* GetVertexBuffer() const { return mVertexBuffer.get(); }
		inline auto* GetIndexBuffer()  const { return mIndexBuffer.get();  }

	private:
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };

		uint32_t mVertexCount{ 0u };
		uint32_t mIndexCount{ 0u };
		std::unique_ptr<Buffer> mVertexBuffer;
		std::unique_ptr<Buffer> mIndexBuffer;
	};

}