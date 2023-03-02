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
		// Archetype中的所有的Chunk都已被填满
		return NewChunk(archetype);
	}

	Chunk* Entity::NewChunk(Archetype& archetype) {
		Chunk* pChunk = new Chunk;
		pChunk->header.ownArchetype = &archetype;
		// 设置当前Chunk的前驱节点
		pChunk->header.prev = archetype.chunkListTail;
		pChunk->header.next = nullptr;
		// 为前驱节点设置其后继
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
		// 删除组件数据的同时，如有必要，对组件数据进行填充
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
		// 删除EntityID标记
		Entity::ID* entityIDArray = reinterpret_cast<Entity::ID*>(pChunk->storage);
		entityIDArray[chunkIndex] = Entity::ID{ -1 };
		if (needPadding) {
			Entity::ID padEntityID = entityIDArray[paddingIndex];
			entityIDArray[paddingIndex] = Entity::ID{ -1 };
			entityIDArray[chunkIndex] = padEntityID;
			// 更新padEntity的Storage
			sEntityStorageArray[padEntityID].chunkIndex = chunkIndex;
		}
		pChunk->header.validCount--;
		if (pChunk->header.validCount == 0) DelChunk(pChunk);
	}
}