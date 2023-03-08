#pragma once
#include "IAsset.h"
#include "IAssetManger.h"
#include "Actor.h"

namespace Core {

	/*
	* Scene是资产管理的顶层组件，它负责管理与该场景相关的Actor
	*/
	class Scene : public IAsset {
	public:
		/*
		* 构造函数
		*/
		Scene(IAssetManger<Scene>* manger);

		/*
		* 析构函数
		*/
		~Scene() = default;

		/*
		* 加载
		*/
		void Load(bool aSync = false);

		/*
		* 卸载
		*/
		void Unload();


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
		* 序列化为Json数据
		*/
		void SerializeJson(Tool::JsonWriter& writer) const;

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const Tool::JsonReader& reader);

	private:
		IAssetManger<Scene>*				mManger{ nullptr };	// 管理器
		int64_t								mActorIncID{ 0 };	// 物体自增ID
		std::vector<std::unique_ptr<Actor>> mActors;			// 场景物体
	};
}