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
		// 提取原生类型
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

		// 构造新的compslist
		const Metatype* tempTypes[32];
		uint32_t len{ 0u };
		for (const auto& pair : srcArchetype.header.pairArray) {
			// 组件已存在
			if (joinedType == pair.metatype) return GetComponent<Comp>();
			tempTypes[len] = pair.metatype;
			len++;
		}
		tempTypes[len] = joinedType;
		len++;

		// 计算新的hashVal
		size_t hashVal = (srcArchetype.hash ^ joinedType->hash);
		if (sArchetypeMap.find(hashVal) == sArchetypeMap.end()) {
			SortByAddress<Metatype>(tempTypes, len);
			size_t compsByteSize = srcArchetype.header.byteSize + joinedType->size;
			NewArchetype(tempTypes, len, hashVal, compsByteSize);
		}
		Archetype& dstArchetype = sArchetypeMap[hashVal];

		// 获得dstChunk与srcChunk
		Chunk* pDstChunk = GetFreeChunk(dstArchetype);
		int dstIndex = AllocateEntity(pDstChunk, *this);
		Chunk* pSrcChunk = sEntityStorageArray[mID].pChunk;
		int srcIndex = sEntityStorageArray[mID].chunkIndex;

		// 构造复制信息表
		for (const auto& srcPair : srcArchetype.header.pairArray) {
			const Metatype* srcType = srcPair.metatype;
			for (const auto& dstPair : dstArchetype.header.pairArray) {
				const Metatype* dstType = dstPair.metatype;
				if (srcType == dstType) {
					// 完成数据复制操作
					void* srcPtr = (void*)(pSrcChunk->storage + srcPair.chunkOffset + srcType->size * srcIndex);
					void* dstPtr = (void*)(pDstChunk->storage + dstPair.chunkOffset + dstType->size * dstIndex);
					memcpy(dstPtr, srcPtr, srcType->size);
				}
				if (dstType == joinedType) {
					// 完成新组件的初始化操作
					dstType->constructor((void*)(pDstChunk->storage + dstPair.chunkOffset + dstType->size * dstIndex));
				}
			}
		}

		// 将实体数据从srcChunk中删除
		DeallocateEntity(pSrcChunk, srcIndex);

		// 更新Entity的EntityStorage
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

		// 构造新的compslist
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

		// 获得dstChunk与srcChunk
		Chunk* pDstChunk = GetFreeChunk(dstArchetype);
		int dstIndex = AllocateEntity(pDstChunk, *this);
		Chunk* pSrcChunk = sEntityStorageArray[mID].pChunk;
		int srcIndex = sEntityStorageArray[mID].chunkIndex;

		for (const auto& srcPair : srcArchetype.header.pairArray) {
			const Metatype* srcType = srcPair.metatype;
			for (const auto& dstPair : dstArchetype.header.pairArray) {
				const Metatype* dstType = dstPair.metatype;
				if (srcType == dstType) {
					// 完成数据复制操作
					void* srcPtr = (void*)(pSrcChunk->storage + srcPair.chunkOffset + srcType->size * srcIndex);
					void* dstPtr = (void*)(pDstChunk->storage + dstPair.chunkOffset + dstType->size * dstIndex);
					memcpy(dstPtr, srcPtr, srcType->size);
				}
			}
		}

		// 将实体数据从srcChunk中删除
		DeallocateEntity(pSrcChunk, srcIndex);

		// 更新Entity的EntityStorage
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
				// 从参数包中解算出Metatypes，并对其排序
				const Metatype* types[] = { SolveMetatype<Comps>()... };
				size_t nums = sizeof(types) / sizeof(Metatype*);
				// 在编译期计算参数包的总字节大小
				constexpr size_t compsByteSize = Metatype::CalByteSize<Comps...>() + sizeof(Entity::ID);
				SortByAddress<Metatype>(types, nums);
				// 创建并存储新的Archetype
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

			// 为Entity分配实际的存储空间
			Chunk* chunk = GetFreeChunk(archetype);
			int index = AllocateEntity(chunk, entity);
			sEntityStorageArray[entity.mID].pChunk = chunk;
			sEntityStorageArray[entity.mID].chunkIndex = index;

			// 对存储空间进行初始化
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
			// archetype没有Chunk
			if (archetype.chunkListHead == nullptr) continue;
			// 检测archetype与lambda是否匹配
			bool isMatch{ true };
			for (uint32_t i = 0; i < processor.sArity; i++) {
				if (!archetype.header.HasMetatype(processor.sHashArray[i])) {
					isMatch = false;
					break;
				}
			}
			// 遍历archetype下的所有Chunk
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
		// 编译期计算Hash值
		constexpr size_t nameHash = MetatypeHashHelper::Build<Comp>();
		if (sMetatypeMap.find(nameHash) == sMetatypeMap.end()) {
			// 编译期计算Metatype
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