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
	* 组件数组
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
	* 组件数组帮助类
	*/
	template<typename Comp>
	struct ComponentArrayBuildHelper {
		/*
		* 构造ComponentArray
		*/
		static auto Build(Chunk* chunk);
	};

	template<typename F>
	struct Processor : public Processor<decltype(&F::operator())> {};

	/*
	* ClassType	:	lambda所对应的匿名类
	* ReturnType:	lambda返回值类型
	* ...Args	:	lambda参数列表
	*/
	template<typename ClassType, typename ReturnType, typename ...Args>
	struct Processor<ReturnType(ClassType::*)(Args...) const> {
		static constexpr int sArity = sizeof...(Args);								// lambda参数个数;
		static constexpr size_t sHashArray[] = { MetatypeHashHelper::Build<Args>()... };		// lambda参数所对应的反射对象的哈希值
		Tool::TaskProxy proxy;	// 任务代理

		/*
		* 添加任务
		*/
		void AddTask(Chunk* chunk, ClassType& lambda) {
			auto tup = std::make_tuple(ComponentArrayBuildHelper<Args>::Build(chunk)...);
			for (int32_t i = chunk->header.validCount - 1; i >= 0; i--) {
				// 注意！！！：必须要加上std::ref，这是因为std::bind默认会将值复制一份再传递。有些任务内需要更改组件的值，值传递的方式会导致错误
				proxy.AddTask(std::bind(lambda, std::ref(std::get<decltype(ComponentArrayBuildHelper<Args>::Build(chunk))>(tup)[i])...));
			}
		}

		/*
		* 添加任务
		*/
		void AddTaskEx(Chunk* chunk, ClassType& lambda, JPH::JobSystem* jobSystem, JPH::JobSystem::Barrier* barrier) {
			auto tup = std::make_tuple(ComponentArrayBuildHelper<Args>::Build(chunk)...);
			for (int32_t i = chunk->header.validCount - 1; i >= 0; i--) {
				// 注意！！！：必须要加上std::ref，这是因为std::bind默认会将值复制一份再传递。有些任务内需要更改组件的值，值传递的方式会导致错误
				JPH::JobHandle jobHandle = jobSystem->CreateJob("Xaun!", JPH::Color::sGreen, std::bind(lambda, std::ref(std::get<decltype(ComponentArrayBuildHelper<Args>::Build(chunk))>(tup)[i])...));
				barrier->AddJobs(&jobHandle, 1u);
			}
		}

		/*
		* 运行任务
		*/
		void RunAllTask() {
			proxy.RunAllTask();
		}

		/*
		* 在当前线程运行任务
		*/
		void RunAllTaskInCurrentThread() {
			proxy.RunAllTaskInCurrentThread();
		}
	};

	template<typename ...Comps>
	struct Counter {
		static constexpr int sArity = sizeof...(Comps);								// lambda参数个数;
		static constexpr size_t sHashArray[] = { MetatypeHashHelper::Build<Comps>()... };		// lambda参数所对应的反射对象的哈希值
		uint32_t itemCount = 0u;

		/*
		* 元素计数
		*/
		void Tick(Chunk* chunk) {
			itemCount += (chunk->header.validCount);
		}
	};

	class Entity {
	public:
		/*
		* 实体ID
		*/
		using ID = int32_t;

		/*
		* 数据索引与杂项信息
		*/
		struct EntityStorage {
		public:
			uint32_t				version{ 0u };		// 实体版本号
			bool					isActive{ false };	// 是否被激活
			Chunk*                  pChunk{ nullptr };	// 指向实体数据所在的Chunk
			uint32_t				chunkIndex{ 0u };	// 实体在Chunk中的索引
			int32_t					parentID{ -1 };		// 父实体ID
			std::vector<int32_t>	childs;				// 子实体ID数组

			bool operator==(const EntityStorage& other) const {
				return pChunk == other.pChunk && chunkIndex == other.chunkIndex && version == other.version;
			}
			bool operator!=(const EntityStorage& other) const {
				return !(other == *this);
			}
		};

	public:

		/*
		* 设置任务系统
		*/
		static void SetJobSystem(JPH::JobSystem* jobSystem);

		/*
		* 测试使用的构造函数
		*/
		inline Entity(int32_t id = -1) : mID(id) {}

		/*
		* 获取EntityID
		*/
		inline const auto& GetID() const { return mID; }

		/*
		* 添加组件
		*/
		template<typename Comp>
		Comp& AddComponent();

		/*
		* 添加组件
		*/
		template<typename Comp>
		Comp& AddComponent(Comp& comp);

		/*
		* 获得组件
		*/
		template<typename Comp>
		Comp& GetComponent();

		/*
		* 获得组件(const)
		*/
		template<typename Comp>
		const Comp& GetComponent() const;

		/*
		* 删除组件
		*/
		template<typename Comp>
		void DelComponent();

		/*
		* 拥有组件
		*/
		template<typename Comp>
		bool HasComponent() const;

		/*
		* 遍历组件
		*/
		void ForeachComp(std::function<void(IComponent*)>&& lambda) const;

		/*
		* 添加父节点
		*/
		void AttachParent(Entity& parent);

		/*
		* 移除父节点
		*/
		void DetachParent();

		/*
		* 创建新的Entity
		*/
		template<typename ...Comps>
		static Entity Create();

		/*
		* 删除目标Entity
		*/
		static void Delete(Entity& entity);

		/*
		* 遍历目标组件集合
		*/
		template<typename F>
		static void Foreach(F&& task);

		/*
		* 在当前线程内进行遍历
		*/
		template<typename F>
		static void ForeachInCurrentThread(F&& task);

		/*
		* 获取目标组件的个数
		*/
		template<typename ...Comps>
		static uint32_t GetEntityCount();

	private:
		/*
		* 解算组件反射
		*/
		template<typename Comp>
		static const Metatype* SolveMetatype();

		/*
		* 按地址顺序进行排序
		*/
		template<typename T>
		static void SortByAddress(const T** pointerArray, size_t nums);

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
		Entity::ID mID{ -1 };	// 实体的ID不需要持久化存储

	private:
		static JPH::JobSystem*                          sJobSystem;				// 任务系统
		static std::unordered_map<size_t, Metatype>		sMetatypeMap;			// 存储所有的组件反射类对象
		static std::unordered_map<size_t, Archetype>	sArchetypeMap;			// 存储所有的原型
		static std::vector<EntityStorage>				sEntityStorageArray;	// 存储实体详细数据
		static std::queue<Entity::ID>					sDeletedEntities;		// 实体池
	};
}

#include "Entity.inl"