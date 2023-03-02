#pragma once
#include "Archetype.h"
#include <algorithm>
#include <queue>

namespace Core {
	template<typename Comp>
	struct ComponentArray {
	public:
		inline ComponentArray() = default;
		inline ComponentArray(void* data, Chunk* chunk) : data(reinterpret_cast<Comp*>(data)), ownChunk(chunk) {}

		inline const Comp& operator[](size_t index) const {
			return data[index];
		}

		inline Comp& operator[](size_t index) {
			return data[index];
		}

		inline Comp* Begin() { return data; }
		inline Comp* End() { return data + ownChunk->header.validCount; }

		inline const Comp* CBegin() const { return data; }
		inline const Comp* CEnd() const { return data; }

		inline uint32_t Size() const { return ownChunk->header.validCount; }

		/*
		* 构造ComponentArray
		*/
		inline static auto Build(Chunk* chunk) {
			// 提取原生类型
			using sanitizedType = std::remove_const_t<std::remove_reference_t<Comp>>;
			if constexpr (std::is_same<sanitizedType, Entity::ID>::value) {
				return ComponentArray<sanitizedType>((void*)(chunk->storage), chunk);
			}
			else {
				constexpr size_t hashVal = MetatypeHashHelper::Build<sanitizedType>();
				for (const auto& pair : chunk->header.pOwnArcheType->header.pairArray) {
					if (pair.pMetatype->hash == hashVal) {
						return ComponentArray<sanitizedType>((void*)(chunk->storage + pair.chunkOffset), chunk);
					}
				}
			}
			return ComponentArray<sanitizedType>();
		}
	public:
		Comp*	data{ nullptr };
		Chunk*	ownChunk{ nullptr };
	};

	/*
	* 实体
	*/
	class Entity {
	public:
		/*
		* 实体ID
		*/
		using ID = int32_t;

		/*
		* 实体的数据索引信息与杂项信息的存储
		*/
		struct EntityStorage {
		public:
			uint32_t	version{ 0u };		// 实体版本号
			bool		isActive{ false };	// 是否被激活
			Chunk*		pChunk{ nullptr };	// 指向实体数据所在的Chunk
			uint32_t	chunkIndex{ 0u };	// 实体在Chunk中的索引

			bool operator==(const EntityStorage& other) const {
				return pChunk == other.pChunk && chunkIndex == other.chunkIndex;
			}
			bool operator!=(const EntityStorage& other) const {
				return !(other == *this);
			}
		};
	public:
		/*
		* 添加组件
		*/
		template<typename Comp>
		Comp& AddComponent() {
			const Metatype* joinedType = SolveMetatype<Comp>();
			Archetype& srcArchetype = *(sEntityStorageArray[mID].pChunk->header.ownArchetype);

			// 构造新的compslist
			const Metatype* tempTypes[32];
			uint32_t len = 0;
			for (const auto& pair : srcArchetype.header.pairArray) {
				// 组件已存在
				if (joinedType == pair.metatype) return;
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

		/*
		* 添加组件
		*/
		template<typename Comp>
		Comp& AddComponent(Comp& comps) {
			AddComponent<Comp>();
			GetComponent<Comp>() = comps;
		}

		/*
		* 获得组件
		*/
		template<typename Comp>
		Comp& GetComponent() {
			const EntityStorage& storage = sEntityStorageArray[mID];
			ComponentArray<Comp> comps = ComponentArray<Comp>::Build(storage.pChunk);
			return comps[storage.chunkIndex];
		}

		/*
		* 删除组件
		*/
		template<typename Comp>
		void DelComponent();

		/*
		* 创建一个新的Entity
		*/
		template<typename ...Comps>
		static Entity Create() {
			if constexpr (sizeof...(Comps) != 0) {
				constexpr size_t hashVal = MetatypeHashHelper::BuildArray<Comps...>();
				if (sArchetypeMap.find(hashVal) == sArchetypeMap.end()) {
					// 从参数包中解算出Metatypes，并对其排序
					const Metatype* types[] = { SolveMetatype<Comps>()... };
					size_t nums = sizeof(types) / sizeof(Metatype*);
					// 在编译期计算参数包的总字节大小
					constexpr size_t compsByteSize = Metatype::calByteSize<Comps...>() + sizeof(Entity::ID);
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


		/*
		* 删除目标Entity
		*/
		static void Delete(const Entity& entity);

		/*
		* 遍历
		*/
		template<typename F>
		static void Foreach(F&& func);

	private:
		/*
		* 解算组件反射类
		*/
		template<typename Comp>
		static const Metatype* SolveMetatype() {
			// 编译期计算Hash值
			constexpr size_t nameHash = MetatypeHashHelper::build<Comp>();
			if (sMetatypeMap.find(nameHash) == sMetatypeMap.end()) {
				// 编译期计算Metatype
				constexpr Metatype metatype = Metatype::build<Comp>();
				sMetatypeMap[nameHash] = std::move(metatype);
			}
			return &sMetatypeMap[nameHash];
		}

		/*
		* 按地址顺序进行排序
		*/
		template<typename T>
		static void SortByAddress(const T** pointerArray, size_t nums) {
			std::sort(pointerArray, pointerArray + nums,
				[](const T* a, const T* b) {
					return a < b;
				});
		}

		/*
		* 新建一个Archetype
		*/
		static void NewArchetype(const Metatype** types, size_t nums, size_t hashVal, size_t compsByteSize);

		/*
		* 获取有空余空间的Chunk
		*/
		static Chunk* GetFreeChunk(Archetype& archetype);

		/*
		* 新建一个Chunk
		*/
		static Chunk* NewChunk(Archetype& archetype);

		/*
		* 删除指定的Chunk
		*/
		static void DelChunk(Chunk* pChunk);

		/*
		* 在Chunk上分配实体的ID数据
		*/
		static int AllocateEntity(Chunk* pChunk, const Entity& entity);

		/*
		* 将Entity的数据从Chunk上删除
		*/
		static void DeallocateEntity(Chunk* pChunk, uint32_t chunkIndex);
	private:
		Entity::ID	mID{ -1 };

	private:
		static std::unordered_map<size_t, Metatype>		sMetatypeMap;			// 存储所有的组件反射类对象
		static std::unordered_map<size_t, Archetype>	sArchetypeMap;			// 存储所有的原型
		static std::vector<EntityStorage>				sEntityStorageArray;	// 存储实体详细数据
		static std::queue<Entity::ID>					sDeletedEntities;		// 实体池
	};
}