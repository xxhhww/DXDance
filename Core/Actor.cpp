#include "Actor.h"
#include "Transform.h"
#include "FooComponent.h"

namespace Core {
	Actor::Actor(int64_t actorID, const std::string& name) 
	: mActorID(actorID)
	, mName(name) 
	, mEntity(Entity::Create()) {}

	Actor::~Actor() {
		Entity::Delete(mEntity);
	}

	void Actor::AttachParent(Actor& parent) {
		// Attach实体
		mEntity.AttachParent(parent.mEntity);

		// AttachActor
		mParentID = parent.GetID();
		mParent = &parent;
		parent.mChilds.push_back(this);
	}

	void Actor::DetachParent() {
		// Detach实体
		mEntity.DetachParent();

		// DetachActor
		mParent->mChilds.erase(
			std::remove_if(mParent->mChilds.begin(), mParent->mChilds.end(),
				[this](Actor* item) {
					return item == this;
				})
		);
		mParent = nullptr;
		mParentID = -1;
	}

	void Actor::SetName(const std::string& name) {
		mName = name;
	}

	void Actor::SetActive(bool isActive) {
		if (isActive) {
			mParent->SetActive(isActive);
		}
		mActive = isActive;
	}

	void Actor::Destory() {
		mDestoryed = true;
		for (auto& child : mChilds) {
			child->Destory();
		}
	}

	void Actor::SerializeBinary(Tool::OutputMemoryStream& blob) const {
		blob.Write(mActorID);
		blob.Write(mName);
		blob.Write(mActive);
		blob.Write(mDestoryed);
		blob.Write(mParentID);

		// 序列化组件数据
		if(HasComponent<Transform>())
	}

	void Actor::DeserializeBinary(Tool::InputMemoryStream& blob) {
		blob.Read(mActorID);
		blob.Read(mName);
		blob.Read(mActive);
		blob.Read(mDestoryed);
		blob.Read(mParentID);

		// 反序列化组件数据

	}

	void Actor::SerializeJson(rapidjson::Document& doc) const {

	}

	void Actor::DeserializeJson(const rapidjson::Document& doc) {

	}
}