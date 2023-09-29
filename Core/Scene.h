#pragma once
#include "Actor.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

namespace Core {

	class SceneManger;

	/*
	* Scene是资产管理的顶层组件，它负责管理与该场景相关的Actor
	*/
	class Scene {
		friend class SceneManger;
	public:
		Scene(SceneManger* manger, int64_t uid);
		~Scene() = default;

		/*
		* 加载
		*/
		void Load();

		/*
		* 卸载
		*/
		void Unload();

		/*
		* 保存到磁盘
		*/
		void SaveToDisk();

		/*
		* 创建Actor(提供的名称应该是独特的)
		*/
		Actor* CreateActor(const std::string& name);

		/*
		* 删除Actor
		*/
		void DeleteActor(Actor* actor);

		/*
		* 通过ID寻找Actor
		*/
		Actor* FindActorByID(int32_t id);

		/*
		* 通过名称寻找Actor
		*/
		Actor* FindActorByName(const std::string& name);

	public:
		inline const auto& GetUID() const { return mUID; }

	public:
		/*
		* 序列化为Json数据
		*/
		void SerializeJson(Tool::JsonWriter& writer) const;

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const Tool::JsonReader& reader);

	public:
		ECS::Camera	   editorCamera;
		ECS::Transform editorTransform;

	private:
		// Scene类或者说资产类中不存放该对象的路径，而是存放资产UID
		// 因为对象的路径是可变的，这属于编辑层的任务，路径变换时修改服务层中的资产路径表即可 
		// 当资产类对象内部需要使用该对象的路径时，查询资产路径表即可

		int64_t								mUID{ 0 };			// 资产UID
		SceneManger*						mManger{ nullptr };	// 资产管理器

		int64_t								mActorIncID{ 0 };	// 物体自增ID
		std::vector<std::unique_ptr<Actor>> mActors;			// 场景物体
	};

}