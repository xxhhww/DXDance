#include "Entity.h"
#include <assert.h>

namespace Core {
	std::unordered_map<size_t, Metatype>	Entity::sMetatypeMap;
	std::unordered_map<size_t, Archetype>	Entity::sArchetypeMap;
	std::vector<Entity::EntityStorage>		Entity::sEntityStorageArray;
	std::queue<Entity::ID>					Entity::sDeletedEntities;

	void Entity::ForeachComp(std::function<void(IComponent*)>&& lambda) const {
		const auto& storage = sEntityStorageArray[mID];
		Chunk* chunk = storage.pChunk;

		for (const auto& pair : chunk->header.ownArchetype->header.pairArray) {
			// 跳过EntityID
			const Metatype* metatype = pair.metatype;
			if (metatype->hash == MetatypeHashHelper::Build<Entity::ID>()) {
				continue;
			}
			void* data = chunk->storage + pair.chunkOffset + metatype->size * storage.chunkIndex;
			lambda((IComponent*)data);
		}
	}

	void Entity::AttachParent(Entity& parent) {
		auto& parentStorage = sEntityStorageArray[parent.mID];
		auto& myStorage = sEntityStorageArray[mID];

		if (parent.mID == myStorage.parentID) {
			return;
		}

		if (parentStorage.isActive && myStorage.isActive) {
			parentStorage.childs.emplace_back(mID);
			myStorage.parentID = parent.mID;
		}
	}

	void Entity::DetachParent() {
		auto& myStorage = sEntityStorageArray[mID];
		if (myStorage.parentID == -1) return;

		auto& parentStorage = sEntityStorageArray[myStorage.parentID];

		if (myStorage.isActive && parentStorage.isActive) {
			parentStorage.childs.erase(
				std::remove_if(parentStorage.childs.begin(), parentStorage.childs.end(),
					[this](int32_t id) {
						return id == mID;
					})
			);
			myStorage.parentID = -1;
		}
	}


	void Entity::Delete(Entity& entity) {
		auto& storage = sEntityStorageArray[entity.mID];

		for (auto& id : storage.childs) {
			Entity child{ id };
			Entity::Delete(child);
		}
		storage.childs.clear();
		entity.DetachParent();

		DeallocateEntity(storage.pChunk, storage.chunkIndex);
		storage.isActive = false;
		storage.pChunk = nullptr;
		storage.chunkIndex = 0u;
		sDeletedEntities.push(entity.mID);
		entity.mID = -1;
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