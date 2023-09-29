#pragma once
#include "Actor.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

namespace Core {

	class WorldManger;

	class World {
		friend class WorldManger;
	public:
		World(WorldManger* manger);
		~World() = default;

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
		WorldManger* mManger{ nullptr };				// 资产管理器
		int64_t	mActorIncID{ 0 };						// 物体自增ID
		std::vector<std::unique_ptr<Actor>> mActors;	// 场景物体

	};

}