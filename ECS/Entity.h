#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include "Archetype.h"
#include "IComponent.h"
#include "Tools/TaskProxy.h"
#include <queue>
#include <algorithm>
#include <unordered_map>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS


namespace ECS {
	/*
	* �������
	*/
	template<typename Comp>
	struct ComponentArray {
	public:
		ComponentArray() = default;
		ComponentArray(void* data, Chunk* chunk);

		const Comp& operator[](size_t index) const;
		Comp& operator[](size_t index);

		Comp* Begin();
		Comp* End();

		const Comp* CBegin() const;
		const Comp* CEnd()   const;

		uint32_t Size() const;

	public:
		Comp* data{ nullptr };
		Chunk* ownChunk{ nullptr };
	};

	/*
	* ������������
	*/
	template<typename Comp>
	struct ComponentArrayBuildHelper {
		/*
		* ����ComponentArray
		*/
		static auto Build(Chunk* chunk);
	};

	template<typename F>
	struct Processor : public Processor<decltype(&F::operator())> {};

	/*
	* ClassType	:	lambda����Ӧ��������
	* ReturnType:	lambda����ֵ����
	* ...Args	:	lambda�����б�
	*/
	template<typename ClassType, typename ReturnType, typename ...Args>
	struct Processor<ReturnType(ClassType::*)(Args...) const> {
		static constexpr int sArity = sizeof...(Args);								// lambda��������;
		static constexpr size_t sHashArray[] = { MetatypeHashHelper::Build<Args>()... };		// lambda��������Ӧ�ķ������Ĺ�ϣֵ
		Tool::TaskProxy proxy;	// �������

		/*
		* �������
		*/
		void AddTask(Chunk* chunk, ClassType& lambda) {
			auto tup = std::make_tuple(ComponentArrayBuildHelper<Args>::Build(chunk)...);
			for (int32_t i = chunk->header.validCount - 1; i >= 0; i--) {
				// ע�⣡����������Ҫ����std::ref��������Ϊstd::bindĬ�ϻὫֵ����һ���ٴ��ݡ���Щ��������Ҫ���������ֵ��ֵ���ݵķ�ʽ�ᵼ�´���
				proxy.AddTask(std::bind(lambda, std::ref(std::get<decltype(ComponentArrayBuildHelper<Args>::Build(chunk))>(tup)[i])...));
			}
		}

		/*
		* �������
		*/
		void AddTaskEx(Chunk* chunk, ClassType& lambda, JPH::JobSystem* jobSystem, JPH::JobSystem::Barrier* barrier) {
			auto tup = std::make_tuple(ComponentArrayBuildHelper<Args>::Build(chunk)...);
			for (int32_t i = chunk->header.validCount - 1; i >= 0; i--) {
				// ע�⣡����������Ҫ����std::ref��������Ϊstd::bindĬ�ϻὫֵ����һ���ٴ��ݡ���Щ��������Ҫ���������ֵ��ֵ���ݵķ�ʽ�ᵼ�´���
				JPH::JobHandle jobHandle = jobSystem->CreateJob("Xaun!", JPH::Color::sGreen, std::bind(lambda, std::ref(std::get<decltype(ComponentArrayBuildHelper<Args>::Build(chunk))>(tup)[i])...));
				barrier->AddJobs(&jobHandle, 1u);
			}
		}

		/*
		* ��������
		*/
		void RunAllTask() {
			proxy.RunAllTask();
		}

		/*
		* �ڵ�ǰ�߳���������
		*/
		void RunAllTaskInCurrentThread() {
			proxy.RunAllTaskInCurrentThread();
		}
	};

	template<typename ...Comps>
	struct Counter {
		static constexpr int sArity = sizeof...(Comps);								// lambda��������;
		static constexpr size_t sHashArray[] = { MetatypeHashHelper::Build<Comps>()... };		// lambda��������Ӧ�ķ������Ĺ�ϣֵ
		uint32_t itemCount = 0u;

		/*
		* Ԫ�ؼ���
		*/
		void Tick(Chunk* chunk) {
			itemCount += (chunk->header.validCount);
		}
	};

	class Entity {
	public:
		/*
		* ʵ��ID
		*/
		using ID = int32_t;

		/*
		* ����������������Ϣ
		*/
		struct EntityStorage {
		public:
			uint32_t				version{ 0u };		// ʵ��汾��
			bool					isActive{ false };	// �Ƿ񱻼���
			Chunk*                  pChunk{ nullptr };	// ָ��ʵ���������ڵ�Chunk
			uint32_t				chunkIndex{ 0u };	// ʵ����Chunk�е�����
			int32_t					parentID{ -1 };		// ��ʵ��ID
			std::vector<int32_t>	childs;				// ��ʵ��ID����

			bool operator==(const EntityStorage& other) const {
				return pChunk == other.pChunk && chunkIndex == other.chunkIndex && version == other.version;
			}
			bool operator!=(const EntityStorage& other) const {
				return !(other == *this);
			}
		};

	public:

		/*
		* ��������ϵͳ
		*/
		static void SetJobSystem(JPH::JobSystem* jobSystem);

		/*
		* ����ʹ�õĹ��캯��
		*/
		inline Entity(int32_t id = -1) : mID(id) {}

		/*
		* ��ȡEntityID
		*/
		inline const auto& GetID() const { return mID; }

		/*
		* ������
		*/
		template<typename Comp>
		Comp& AddComponent();

		/*
		* ������
		*/
		template<typename Comp>
		Comp& AddComponent(Comp& comp);

		/*
		* ������
		*/
		template<typename Comp>
		Comp& GetComponent();

		/*
		* ������(const)
		*/
		template<typename Comp>
		const Comp& GetComponent() const;

		/*
		* ɾ�����
		*/
		template<typename Comp>
		void DelComponent();

		/*
		* ӵ�����
		*/
		template<typename Comp>
		bool HasComponent() const;

		/*
		* �������
		*/
		void ForeachComp(std::function<void(IComponent*)>&& lambda) const;

		/*
		* ��Ӹ��ڵ�
		*/
		void AttachParent(Entity& parent);

		/*
		* �Ƴ����ڵ�
		*/
		void DetachParent();

		/*
		* �����µ�Entity
		*/
		template<typename ...Comps>
		static Entity Create();

		/*
		* ɾ��Ŀ��Entity
		*/
		static void Delete(Entity& entity);

		/*
		* ����Ŀ���������
		*/
		template<typename F>
		static void Foreach(F&& task);

		/*
		* �ڵ�ǰ�߳��ڽ��б���
		*/
		template<typename F>
		static void ForeachInCurrentThread(F&& task);

		/*
		* ��ȡĿ������ĸ���
		*/
		template<typename ...Comps>
		static uint32_t GetEntityCount();

	private:
		/*
		* �����������
		*/
		template<typename Comp>
		static const Metatype* SolveMetatype();

		/*
		* ����ַ˳���������
		*/
		template<typename T>
		static void SortByAddress(const T** pointerArray, size_t nums);

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
		Entity::ID mID{ -1 };	// ʵ���ID����Ҫ�־û��洢

	private:
		static JPH::JobSystem*                          sJobSystem;				// ����ϵͳ
		static std::unordered_map<size_t, Metatype>		sMetatypeMap;			// �洢���е�������������
		static std::unordered_map<size_t, Archetype>	sArchetypeMap;			// �洢���е�ԭ��
		static std::vector<EntityStorage>				sEntityStorageArray;	// �洢ʵ����ϸ����
		static std::queue<Entity::ID>					sDeletedEntities;		// ʵ���
	};
}

#include "Entity.inl"