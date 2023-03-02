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
		* ����ComponentArray
		*/
		inline static auto Build(Chunk* chunk) {
			// ��ȡԭ������
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
	* ʵ��
	*/
	class Entity {
	public:
		/*
		* ʵ��ID
		*/
		using ID = int32_t;

		/*
		* ʵ�������������Ϣ��������Ϣ�Ĵ洢
		*/
		struct EntityStorage {
		public:
			uint32_t	version{ 0u };		// ʵ��汾��
			bool		isActive{ false };	// �Ƿ񱻼���
			Chunk*		pChunk{ nullptr };	// ָ��ʵ���������ڵ�Chunk
			uint32_t	chunkIndex{ 0u };	// ʵ����Chunk�е�����

			bool operator==(const EntityStorage& other) const {
				return pChunk == other.pChunk && chunkIndex == other.chunkIndex;
			}
			bool operator!=(const EntityStorage& other) const {
				return !(other == *this);
			}
		};
	public:
		/*
		* ������
		*/
		template<typename Comp>
		Comp& AddComponent() {
			const Metatype* joinedType = SolveMetatype<Comp>();
			Archetype& srcArchetype = *(sEntityStorageArray[mID].pChunk->header.ownArchetype);

			// �����µ�compslist
			const Metatype* tempTypes[32];
			uint32_t len = 0;
			for (const auto& pair : srcArchetype.header.pairArray) {
				// ����Ѵ���
				if (joinedType == pair.metatype) return;
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

		/*
		* ������
		*/
		template<typename Comp>
		Comp& AddComponent(Comp& comps) {
			AddComponent<Comp>();
			GetComponent<Comp>() = comps;
		}

		/*
		* ������
		*/
		template<typename Comp>
		Comp& GetComponent() {
			const EntityStorage& storage = sEntityStorageArray[mID];
			ComponentArray<Comp> comps = ComponentArray<Comp>::Build(storage.pChunk);
			return comps[storage.chunkIndex];
		}

		/*
		* ɾ�����
		*/
		template<typename Comp>
		void DelComponent();

		/*
		* ����һ���µ�Entity
		*/
		template<typename ...Comps>
		static Entity Create() {
			if constexpr (sizeof...(Comps) != 0) {
				constexpr size_t hashVal = MetatypeHashHelper::BuildArray<Comps...>();
				if (sArchetypeMap.find(hashVal) == sArchetypeMap.end()) {
					// �Ӳ������н����Metatypes������������
					const Metatype* types[] = { SolveMetatype<Comps>()... };
					size_t nums = sizeof(types) / sizeof(Metatype*);
					// �ڱ����ڼ�������������ֽڴ�С
					constexpr size_t compsByteSize = Metatype::calByteSize<Comps...>() + sizeof(Entity::ID);
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


		/*
		* ɾ��Ŀ��Entity
		*/
		static void Delete(const Entity& entity);

		/*
		* ����
		*/
		template<typename F>
		static void Foreach(F&& func);

	private:
		/*
		* �������������
		*/
		template<typename Comp>
		static const Metatype* SolveMetatype() {
			// �����ڼ���Hashֵ
			constexpr size_t nameHash = MetatypeHashHelper::build<Comp>();
			if (sMetatypeMap.find(nameHash) == sMetatypeMap.end()) {
				// �����ڼ���Metatype
				constexpr Metatype metatype = Metatype::build<Comp>();
				sMetatypeMap[nameHash] = std::move(metatype);
			}
			return &sMetatypeMap[nameHash];
		}

		/*
		* ����ַ˳���������
		*/
		template<typename T>
		static void SortByAddress(const T** pointerArray, size_t nums) {
			std::sort(pointerArray, pointerArray + nums,
				[](const T* a, const T* b) {
					return a < b;
				});
		}

		/*
		* �½�һ��Archetype
		*/
		static void NewArchetype(const Metatype** types, size_t nums, size_t hashVal, size_t compsByteSize);

		/*
		* ��ȡ�п���ռ��Chunk
		*/
		static Chunk* GetFreeChunk(Archetype& archetype);

		/*
		* �½�һ��Chunk
		*/
		static Chunk* NewChunk(Archetype& archetype);

		/*
		* ɾ��ָ����Chunk
		*/
		static void DelChunk(Chunk* pChunk);

		/*
		* ��Chunk�Ϸ���ʵ���ID����
		*/
		static int AllocateEntity(Chunk* pChunk, const Entity& entity);

		/*
		* ��Entity�����ݴ�Chunk��ɾ��
		*/
		static void DeallocateEntity(Chunk* pChunk, uint32_t chunkIndex);
	private:
		Entity::ID	mID{ -1 };

	private:
		static std::unordered_map<size_t, Metatype>		sMetatypeMap;			// �洢���е�������������
		static std::unordered_map<size_t, Archetype>	sArchetypeMap;			// �洢���е�ԭ��
		static std::vector<EntityStorage>				sEntityStorageArray;	// �洢ʵ����ϸ����
		static std::queue<Entity::ID>					sDeletedEntities;		// ʵ���
	};
}