#pragma once
#include "Entity.h"

namespace Core {
	template<typename Comp>
	ComponentArray<Comp>::ComponentArray(void* data, Chunk* chunk) : data(reinterpret_cast<Comp*>(data)), ownChunk(chunk) {}

	template<typename Comp>
	const Comp& ComponentArray<Comp>::operator[](size_t index) const {
		return data[index];
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::operator[](size_t index) {
		return data[index];
	}

	template<typename Comp>
	Comp* ComponentArray<Comp>::Begin() { return data; }

	template<typename Comp>
	Comp* ComponentArray<Comp>::End() { return data + ownChunk->header.validCount; }

	template<typename Comp>
	const Comp* ComponentArray<Comp>::CBegin() const { return data; }
	
	template<typename Comp>
	const Comp* ComponentArray<Comp>::CEnd() const { return data; }

	template<typename Comp>
	uint32_t ComponentArray<Comp>::Size() const { return ownChunk->header.validCount; }

	template<typename Comp>
	auto ComponentArrayBuildHelper<Comp>::Build(Chunk* chunk) {
		// ��ȡԭ������
		using sanitizedType = std::remove_const_t<std::remove_reference_t<Comp>>;
		if constexpr (std::is_same<sanitizedType, Entity::ID>::value) {
			return ComponentArray<sanitizedType>((void*)(chunk->storage), chunk);
		}
		else {
			constexpr size_t hashVal = MetatypeHashHelper::Build<sanitizedType>();
			for (const auto& pair : chunk->header.ownArchetype->header.pairArray) {
				if (pair.metatype->hash == hashVal) {
					return ComponentArray<sanitizedType>((void*)(chunk->storage + pair.chunkOffset), chunk);
				}
			}
		}
		return ComponentArray<sanitizedType>();
	}

	template<typename Comp>
	Comp& Entity::AddComponent() {
		const Metatype* joinedType = SolveMetatype<Comp>();
		Archetype& srcArchetype = *(sEntityStorageArray[mID].pChunk->header.ownArchetype);

		// �����µ�compslist
		const Metatype* tempTypes[32];
		uint32_t len{ 0u };
		for (const auto& pair : srcArchetype.header.pairArray) {
			// ����Ѵ���
			if (joinedType == pair.metatype) return GetComponent<Comp>();
			tempTypes[len] = pair.metatype;
			len++;
		}
		tempTypes[len] = joinedType;
		len++;

		// �����µ�hashVal
		size_t hashVal = (srcArchetype.hash ^ joinedType->hash);
		if (sArchetypeMap.find(hashVal) == sArchetypeMap.end()) {
			SortByAddress<Metatype>(tempTypes, len);
			size_t compsByteSize = srcArchetype.header.byteSize + joinedType->size;
			NewArchetype(tempTypes, len, hashVal, compsByteSize);
		}
		Archetype& dstArchetype = sArchetypeMap[hashVal];

		// ���dstChunk��srcChunk
		Chunk* pDstChunk = GetFreeChunk(dstArchetype);
		int dstIndex = AllocateEntity(pDstChunk, *this);
		Chunk* pSrcChunk = sEntityStorageArray[mID].pChunk;
		int srcIndex = sEntityStorageArray[mID].chunkIndex;

		// ���츴����Ϣ��
		for (const auto& srcPair : srcArchetype.header.pairArray) {
			const Metatype* srcType = srcPair.metatype;
			for (const auto& dstPair : dstArchetype.header.pairArray) {
				const Metatype* dstType = dstPair.metatype;
				if (srcType == dstType) {
					// ������ݸ��Ʋ���
					void* srcPtr = (void*)(pSrcChunk->storage + srcPair.chunkOffset + srcType->size * srcIndex);
					void* dstPtr = (void*)(pDstChunk->storage + dstPair.chunkOffset + dstType->size * dstIndex);
					memcpy(dstPtr, srcPtr, srcType->size);
				}
				if (dstType == joinedType) {
					// ���������ĳ�ʼ������
					dstType->constructor((void*)(pDstChunk->storage + dstPair.chunkOffset + dstType->size * dstIndex));
				}
			}
		}

		// ��ʵ�����ݴ�srcChunk��ɾ��
		DeallocateEntity(pSrcChunk, srcIndex);

		// ����Entity��EntityStorage
		sEntityStorageArray[mID].pChunk = pDstChunk;
		sEntityStorageArray[mID].chunkIndex = dstIndex;

		return GetComponent<Comp>();
	}

	template<typename Comp>
	Comp& Entity::AddComponent(Comp& comp) {
		AddComponent<Comp>() = comp;
		return comp;
	}

	template<typename Comp>
	Comp& Entity::GetComponent() {
		const EntityStorage& storage = sEntityStorageArray[mID];
		ComponentArray<Comp> comps = ComponentArrayBuildHelper<Comp>::Build(storage.pChunk);
		return comps[storage.chunkIndex];
	}

	template<typename Comp>
	inline const Comp& Entity::GetComponent() const {
		const EntityStorage& storage = sEntityStorageArray.at(mID);
		ComponentArray<Comp> comps = ComponentArrayBuildHelper<Comp>::Build(storage.pChunk);
		return comps[storage.chunkIndex];
	}

	template<typename Comp>
	void Entity::DelComponent() {
		const Metatype* deletedType = SolveMetatype<Comp>();
		Archetype& srcArchetype = *(sEntityStorageArray[mID].pChunk->header.ownArchetype);

		// �����µ�compslist
		const Metatype* tempTypes[32];
		uint32_t		len{ 0u };
		size_t			hashVal{ 0u };
		bool			isExist{ false };
		for (const auto& pair : srcArchetype.header.pairArray) {
			if (deletedType == pair.metatype) {
				isExist = true;
				continue;
			}
			tempTypes[len] = pair.metatype;
			len++;
			hashVal ^= pair.metatype->hash;
		}
		if (!isExist) {
			return;
		}

		if (sArchetypeMap.find(hashVal) == sArchetypeMap.end()) {
			SortByAddress<Metatype>(tempTypes, len);
			size_t compsByteSize = srcArchetype.header.byteSize - deletedType->size;
			NewArchetype(tempTypes, len, hashVal, compsByteSize);
		}

		Archetype& dstArchetype = sArchetypeMap[hashVal];

		// ���dstChunk��srcChunk
		Chunk* pDstChunk = GetFreeChunk(dstArchetype);
		int dstIndex = AllocateEntity(pDstChunk, *this);
		Chunk* pSrcChunk = sEntityStorageArray[mID].pChunk;
		int srcIndex = sEntityStorageArray[mID].chunkIndex;

		for (const auto& srcPair : srcArchetype.header.pairArray) {
			const Metatype* srcType = srcPair.metatype;
			for (const auto& dstPair : dstArchetype.header.pairArray) {
				const Metatype* dstType = dstPair.metatype;
				if (srcType == dstType) {
					// ������ݸ��Ʋ���
					void* srcPtr = (void*)(pSrcChunk->storage + srcPair.chunkOffset + srcType->size * srcIndex);
					void* dstPtr = (void*)(pDstChunk->storage + dstPair.chunkOffset + dstType->size * dstIndex);
					memcpy(dstPtr, srcPtr, srcType->size);
				}
			}
		}

		// ��ʵ�����ݴ�srcChunk��ɾ��
		DeallocateEntity(pSrcChunk, srcIndex);

		// ����Entity��EntityStorage
		sEntityStorageArray[mID].pChunk = pDstChunk;
		sEntityStorageArray[mID].chunkIndex = dstIndex;
	}

	template<typename Comp>
	bool Entity::HasComponent() const {
		const EntityStorage& storage = sEntityStorageArray[mID];
		ComponentArray<Comp> comps = ComponentArrayBuildHelper<Comp>::Build(storage.pChunk);
		return comps.data != nullptr;
	}

	template<typename ...Comps>
	static Entity Entity::Create() {
		if constexpr (sizeof...(Comps) != 0) {
			constexpr size_t hashVal = MetatypeHashHelper::BuildArray<Comps...>();
			if (sArchetypeMap.find(hashVal) == sArchetypeMap.end()) {
				// �Ӳ������н����Metatypes������������
				const Metatype* types[] = { SolveMetatype<Comps>()... };
				size_t nums = sizeof(types) / sizeof(Metatype*);
				// �ڱ����ڼ�������������ֽڴ�С
				constexpr size_t compsByteSize = Metatype::CalByteSize<Comps...>() + sizeof(Entity::ID);
				SortByAddress<Metatype>(types, nums);
				// �������洢�µ�Archetype
				NewArchetype(types, nums, hashVal, compsByteSize);
			}

			Archetype& archetype = sArchetypeMap[hashVal];
			Entity entity;
			if (sDeletedEntities.empty()) {
				entity.mID = (int32_t)sEntityStorageArray.size();
				EntityStorage storage;
				storage.pChunk = nullptr;
				storage.chunkIndex = 0u;
				storage.version = 0u;
				sEntityStorageArray.push_back(std::move(storage));
			}
			else {
				entity.mID = sDeletedEntities.front();
				sDeletedEntities.pop();
				sEntityStorageArray[entity.mID].pChunk = nullptr;
				sEntityStorageArray[entity.mID].version++;
				sEntityStorageArray[entity.mID].chunkIndex = 0u;
			}

			// ΪEntity����ʵ�ʵĴ洢�ռ�
			Chunk* chunk = GetFreeChunk(archetype);
			int index = AllocateEntity(chunk, entity);
			sEntityStorageArray[entity.mID].pChunk = chunk;
			sEntityStorageArray[entity.mID].chunkIndex = index;

			// �Դ洢�ռ���г�ʼ��
			for (const auto& pair : archetype.header.pairArray) {
				const Metatype* type = pair.metatype;
				type->constructor((void*)(chunk->storage + pair.chunkOffset + type->size * index));
			}

			return entity;
		}

		return Entity{};
	}

	template<typename F>
	static void Entity::Foreach(F&& task) {
		Processor<F> processor;
		for (auto& pair : sArchetypeMap) {
			Archetype& archetype = pair.second;
			// archetypeû��Chunk
			if (archetype.chunkListHead == nullptr) continue;
			// ���archetype��lambda�Ƿ�ƥ��
			bool isMatch{ true };
			for (uint32_t i = 0; i < processor.sArity; i++) {
				if (!archetype.header.HasMetatype(processor.sHashArray[i])) {
					isMatch = false;
					break;
				}
			}
			// ����archetype�µ�����Chunk
			if (isMatch) {
				Chunk* pCurrChunk = archetype.chunkListHead;
				while (pCurrChunk != nullptr) {
					processor.AddTask(pCurrChunk, task);
					pCurrChunk = pCurrChunk->header.next;
				}
			}
		}
		processor.RunAllTask();
	}

	template<typename Comp>
	static const Metatype* Entity::SolveMetatype() {
		// �����ڼ���Hashֵ
		constexpr size_t nameHash = MetatypeHashHelper::Build<Comp>();
		if (sMetatypeMap.find(nameHash) == sMetatypeMap.end()) {
			// �����ڼ���Metatype
			constexpr Metatype metatype = Metatype::Build<Comp>();
			sMetatypeMap[nameHash] = std::move(metatype);
		}
		return &sMetatypeMap[nameHash];
	}

	template<typename T>
	static void Entity::SortByAddress(const T** pointerArray, size_t nums) {
		std::sort(pointerArray, pointerArray + nums,
			[](const T* a, const T* b) {
				return a < b;
			});
	}
}