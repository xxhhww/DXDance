#pragma once
#include "Entity.h"
#include "Tools/Event.h"
#include "Tools/ISerializable.h"

namespace Core {
	/*
	* 场景物体
	*/
	class Actor : public Tool::ISerializable {
	public:
		/*
		* 构造函数
		*/
		Actor(const std::string& name);

		/*
		* 析构函数
		*/
		~Actor();

		/*
		* 设置名称
		*/
		void SetName(const std::string& name);

		/*
		* 添加父节点
		*/
		void AttachParent(Actor& parent);

		/*
		* 移除父节点
		*/
		void DetachParent();

		/*
		* Get方法
		*/
		inline const auto& GetName() const { return mName; }
		inline const auto& GetID()   const { return mEntity.GetID(); }
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
		std::string			mName;				// 物体名称(显示在Hirerachy中)
		Actor*				mParent{ nullptr };	// 父物体
		std::vector<Actor*> mChilds;			// 子物体

		Entity				mEntity;			// ECS实体
	};
}