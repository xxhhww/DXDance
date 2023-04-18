#pragma once
#include "Metatype.h"
#include <vector>
#include <rectypes.h>

namespace ECS {
	struct Archetype;
	struct Chunk;

	/*
	* �����ڳ�����Chunk�Ŀ��ô洢��С
	*/
	constexpr size_t CHUNK_MEMORY_SIZE = 32768;

	/*
	* ���ݿ���Ϣ
	*/
	struct ChunkHeader {
	public:
		Archetype* ownArchetype{ nullptr };	// ������Archetype
		Chunk* prev{ nullptr };				// ǰ��
		Chunk* next{ nullptr };				// ���
		uint32_t validCount{ 0u };			// ��ǰ����Ч�洢����
		uint32_t maxCapacity{ 0 };			// ��������
	};

	/*
	* ���ݿ�
	*/
	struct alignas(32) Chunk {
		byte storage[CHUNK_MEMORY_SIZE - sizeof(ChunkHeader)];
		ChunkHeader header;
	};

	/*
	* ԭ����Ϣ
	*/
	struct ArchetypeHeader {
	public:
		struct MetatypePair {
		public:
			const Metatype* metatype{ nullptr };	// ���������
			size_t chunkOffset{ 0u };				// Metatype��������Chunk�ж�Ӧ��λ��
		};

	public:
		/*
		* ����Archetype Header
		*/
		void Create(const Metatype** types, size_t nums, size_t compsByteSize);

		/*
		* �жϸ�ԭ���Ƿ���ж�Ӧ�����
		*/
		bool HasMetatype(size_t hash) const;
	public:
		std::vector<MetatypePair> pairArray;
		size_t byteSize{ 0u };			// ��ԭ�͵���������ֽڴ�С
		uint32_t chunkCapacity{ 0u };	// Chunk������(ͬChunkHeader�е�maxCapacity)
	};

	/*
	* ԭ��
	*/
	struct Archetype {
	public:
		/*
		* Ĭ����������
		*/
		~Archetype();

		/*
		* �����п���ռ��Chunk
		*/
		Chunk* GetFreeChunk();

		/*
		* �����µ�Chunk(�ⲿ��Ӧֱ��ʹ��)
		*/
		Chunk* NewChunk();

		/*
		* ɾ��ָ��Chunk
		*/
		void DelChunk(Chunk* chunk);

		ArchetypeHeader header;
		size_t			hash{ 0u };					// ԭ�͵Ĺ�ϣֵ
		uint32_t		fullChunks{ 0u };			// �ѱ�������Chunk����
		Chunk* chunkListHead{ nullptr };	// Chunk�б�ͷ
		Chunk* chunkListTail{ nullptr };	// Chunk�б�β
	};
}