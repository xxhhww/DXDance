#pragma once
#include "Metatype.h"
#include <vector>
#include <rectypes.h>

namespace ECS {
	struct Archetype;
	struct Chunk;

	/*
	* 编译期常量，Chunk的可用存储大小
	*/
	constexpr size_t CHUNK_MEMORY_SIZE = 32768;

	/*
	* 数据块信息
	*/
	struct ChunkHeader {
	public:
		Archetype* ownArchetype{ nullptr };	// 所属的Archetype
		Chunk* prev{ nullptr };				// 前驱
		Chunk* next{ nullptr };				// 后继
		uint32_t validCount{ 0u };			// 当前的有效存储个数
		uint32_t maxCapacity{ 0 };			// 最大的容量
	};

	/*
	* 数据块
	*/
	struct alignas(32) Chunk {
		byte storage[CHUNK_MEMORY_SIZE - sizeof(ChunkHeader)];
		ChunkHeader header;
	};

	/*
	* 原型信息
	*/
	struct ArchetypeHeader {
	public:
		struct MetatypePair {
		public:
			const Metatype* metatype{ nullptr };	// 组件反射类
			size_t chunkOffset{ 0u };				// Metatype的数据在Chunk中对应的位置
		};

	public:
		/*
		* 创建Archetype Header
		*/
		void Create(const Metatype** types, size_t nums, size_t compsByteSize);

		/*
		* 判断该原型是否具有对应的组件
		*/
		bool HasMetatype(size_t hash) const;
	public:
		std::vector<MetatypePair> pairArray;
		size_t byteSize{ 0u };			// 该原型的组件的总字节大小
		uint32_t chunkCapacity{ 0u };	// Chunk的容量(同ChunkHeader中的maxCapacity)
	};

	/*
	* 原型
	*/
	struct Archetype {
	public:
		/*
		* 默认析构函数
		*/
		~Archetype();

		/*
		* 返回有空余空间的Chunk
		*/
		Chunk* GetFreeChunk();

		/*
		* 创建新的Chunk(外部不应直接使用)
		*/
		Chunk* NewChunk();

		/*
		* 删除指定Chunk
		*/
		void DelChunk(Chunk* chunk);

		ArchetypeHeader header;
		size_t			hash{ 0u };					// 原型的哈希值
		uint32_t		fullChunks{ 0u };			// 已被填满的Chunk个数
		Chunk* chunkListHead{ nullptr };	// Chunk列表头
		Chunk* chunkListTail{ nullptr };	// Chunk列表尾
	};
}