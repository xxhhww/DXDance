#include "Archetype.h"
#include "Entity.h"
#include <assert.h>

namespace Core {

	void ArchetypeHeader::Create(const Metatype** types, size_t nums, size_t compsByteSize) {

		size_t availableStorage = sizeof(Chunk::storage);
		// 2 less than the real count to account for sizes and give some slack
		size_t itemCount = (availableStorage / compsByteSize) - 1;
		assert(itemCount > 0);

		// 实体的id也会被当做组件进行存储
		size_t chunkOffset = sizeof(Entity::ID) * itemCount;
		for (uint32_t i = 0; i < nums; i++) {
			const Metatype* type = types[i];
			if (type->align != 0) {
				// align properly
				size_t remainder = chunkOffset % type->align;
				size_t offset = type->align - remainder;
				chunkOffset += offset;
			}
			this->pairArray.push_back({ type ,chunkOffset });
			if (type->align != 0) {
				chunkOffset += type->size * (itemCount);
			}
		}
		assert(chunkOffset <= availableStorage);
		this->chunkCapacity = (uint32_t)itemCount;
		this->byteSize = compsByteSize;
	}

	bool ArchetypeHeader::HasMetatype(size_t hash) const {
		constexpr size_t entityHashID = MetatypeHashHelper::Build<Entity::ID>();
		if (hash == entityHashID) {
			return true;
		}

		for (const auto& pair : pairArray) {
			if (pair.metatype->hash == hash) {
				return true;
			}
		}

		return false;
	}

	Archetype::~Archetype() {
		while (chunkListHead->header.next != nullptr) {
			chunkListHead = chunkListHead->header.next;
			delete chunkListHead->header.prev;
		}
		delete chunkListHead;
	}
}