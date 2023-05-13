#pragma once
#include "ECS/Entity.h"

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
		Actor(int64_t actorID, const std::string& name);

		/*
		* 析构函数
		*/
		~Actor();

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
		* 获得组件
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
		* 添加父节点
		*/
		void AttachParent(Actor& parent);

		/*
		* 移除父节点
		*/
		void DetachParent();

		/*
		* 设置名称
		*/
		void SetName(const std::string& name);

		/*
		* 设置激活状态。对象被激活时，会将父对象也设置为激活状态，对象失活时则不会
		*/
		void SetActive(bool isActive);

		/*
		* 设置销毁状态。对象被销毁时，会将子对象也设置为销毁状态
		*/
		void Destory();

		/*
		* Get方法
		*/
		inline const auto& GetName()		const { return mName; }
		inline const auto& GetID()			const { return mActorID; }
		inline const auto& GetActive()		const { return mActive; }
		inline const auto& GetDestoryed()	const { return mDestoryed; }
		inline const auto& GetChilds()		const { return mChilds; }
		inline const auto& GetParentID()	const { return mParentID; }
		inline Actor*	   GetParent()		const { return mParent; }
	public:
		/*
		* 序列化为Json数据
		*/
		void SerializeJson(Tool::JsonWriter& writer) const override;

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override;

	public:
		// Actor创建回调，链接到编辑层的Hierarchy
		inline static Tool::Event<int64_t> ActorCreatedEvent;
		// Actor销毁回调，链接到编辑层的Hierarchy
		inline static Tool::Event<int64_t> ActorDeletedEvent;
		
		inline static Tool::Event<int64_t, int64_t> ActorAttachEvent;
		inline static Tool::Event<int64_t>			ActorDetachEvent;

	private:
		int64_t				mActorID{ -1 };		// 物体ID(与Entity的ID不同，ActorID需要持久化存储)
		std::string			mName;				// 物体名称(显示在Hirerachy中)

		bool				mActive{ true };	// 是否激活
		bool				mDestoryed{ false };// 是否销毁

		int64_t				mParentID{ -1 };	// 父物体ID
		Actor*				mParent{ nullptr };	// 父物体
		std::vector<Actor*> mChilds;			// 子物体

		ECS::Entity			mEntity;			// ECS实体
	};
}

#include "Actor.inl"