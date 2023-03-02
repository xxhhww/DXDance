#include "Entity.h"
#include <assert.h>

namespace Core {
	void Entity::Delete(const Entity& entity) {
		auto& storage = sEntityStorageArray[entity.mID];
		DeallocateEntity(storage.pChunk, storage.chunkIndex);
		storage.isActive = false;
		storage.pChunk = nullptr;
		storage.chunkIndex = 0u;
		sDeletedEntities.push(entity.mID);
	}

	void Entity::NewArchetype(const Metatype** types, size_t nums, size_t hashVal, size_t compsByteSize) {
		Archetype& archetype = sArchetypeMap[hashVal];
		archetype.header.Create(types, nums, compsByteSize);
		archetype.hash = hashVal;
		archetype.fullChunks = 0u;
	}

	Chunk* Entity::GetFreeChunk(Archetype& archetype) {
		if (archetype.chunkListHead == nullptr) return NewChunk(archetype);
		Chunk* targetChunk = archetype.chunkListHead;
		while (targetChunk != nullptr) {
			if (targetChunk->header.validCount < targetChunk->header.maxCapacity) return targetChunk;
			targetChunk = targetChunk->header.next;
		}
		// Archetype�е����е�Chunk���ѱ�����
		return NewChunk(archetype);
	}

	Chunk* Entity::NewChunk(Archetype& archetype) {
		Chunk* pChunk = new Chunk;
		pChunk->header.ownArchetype = &archetype;
		// ���õ�ǰChunk��ǰ���ڵ�
		pChunk->header.prev = archetype.chunkListTail;
		pChunk->header.next = nullptr;
		// Ϊǰ���ڵ���������
		if (pChunk->header.prev != nullptr) pChunk->header.prev->header.next = pChunk;
		pChunk->header.validCount = 0u;
		pChunk->header.maxCapacity = archetype.header.chunkCapacity;
		archetype.chunkListTail = pChunk;
		if (archetype.chunkListHead == nullptr) archetype.chunkListHead = pChunk;
		return pChunk;
	}

	void Entity::DelChunk(Chunk* pChunk) {
		if (pChunk->header.prev != nullptr) pChunk->header.prev->header.next = pChunk->header.next;
		else pChunk->header.ownArchetype->chunkListHead = pChunk->header.next;

		if (pChunk->header.next != nullptr) pChunk->header.next->header.prev = pChunk->header.prev;
		else pChunk->header.ownArchetype->chunkListTail = pChunk->header.prev;

		delete pChunk;
	};

	int Entity::AllocateEntity(Chunk* pChunk, const Entity& entity) {
		assert(pChunk->header.validCount < pChunk->header.maxCapacity);
		int entityIndex = pChunk->header.validCount;
		Entity::ID* entityIDArray = reinterpret_cast<Entity::ID*>(pChunk->storage);
		entityIDArray[entityIndex] = entity.mID;
		pChunk->header.validCount++;
		return entityIndex;
	}

	void Entity::DeallocateEntity(Chunk* pChunk, uint32_t chunkIndex) {
		assert(chunkIndex < pChunk->header.validCount);
		bool needPadding = ((pChunk->header.validCount > 1) && (chunkIndex < (pChunk->header.validCount - 1)));
		uint32_t paddingIndex = needPadding ? (pChunk->header.validCount - 1) : 0u;
		// ɾ��������ݵ�ͬʱ�����б�Ҫ����������ݽ������
		for (const auto& pair : pChunk->header.ownArchetype->header.pairArray) {
			const Metatype* type = pair.metatype;
			void* delPtr = (void*)(pChunk->storage + pair.chunkOffset + type->size * chunkIndex);
			type->destructor(delPtr);
			if (needPadding) {
				void* paddingPtr = (void*)(pChunk->storage + pair.chunkOffset + type->size * paddingIndex);
				memcpy(delPtr, paddingPtr, type->size);
				type->destructor(paddingPtr);
			}
		}
		// ɾ��EntityID���
		Entity::ID* entityIDArray = reinterpret_cast<Entity::ID*>(pChunk->storage);
		entityIDArray[chunkIndex] = Entity::ID{ -1 };
		if (needPadding) {
			Entity::ID padEntityID = entityIDArray[paddingIndex];
			entityIDArray[paddingIndex] = Entity::ID{ -1 };
			entityIDArray[chunkIndex] = padEntityID;
			// ����padEntity��Storage
			sEntityStorageArray[padEntityID].chunkIndex = chunkIndex;
		}
		pChunk->header.validCount--;
		if (pChunk->header.validCount == 0) DelChunk(pChunk);
	}
}