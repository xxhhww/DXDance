#pragma once
#include "IAsset.h"
#include "Actor.h"

namespace Core {

	/*
	* Scene(场景)是资产管理的顶层组件，它负责管理与该场景相关的Actor(场景物体，也即二级ECS组件)
	*/
	class Scene : public IAsset {
	public:
		/*
		* 构造函数
		*/
		Scene() = default;

		/*
		* 析构函数
		*/
		~Scene() = default;

		/*
		* 创建Actor(提供的名称应该是独特的)
		*/
		Actor* CreateActor(const std::string& name);

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
		* 序列化为二进制数据
		*/
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override {}

		/*
		* 反序列化二进制数据
		*/
		void DeserializeBinary(const Tool::InputMemoryStream& blob) override {}

		/*
		* 序列化为Json数据
		*/
		void SerializeJson(rapidjson::Document& doc) const override;

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const rapidjson::Document& doc) override;

	private:
		bool								mPlaying{ false };	// play模式
		std::vector<std::unique_ptr<Actor>> mActors;			// 场景物体
	};
}