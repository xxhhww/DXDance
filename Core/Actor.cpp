#include "Actor.h"

namespace Core {
	Actor::Actor(const std::string& name) 
	: mName(name) 
	, mEntity(Entity::Create()) {}

	Actor::~Actor() {
		Entity::Delete(mEntity);
	}

	void Actor::SetName(const std::string& name) {
		mName = name;
	}

	void Actor::AttachParent(Actor& parent) {
		// Attach实体
		mEntity.AttachParent(parent.mEntity);

		// AttachActor
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
	}

	void Actor::SerializeJson(rapidjson::Document& doc) const {

	}

	void Actor::DeserializeJson(const rapidjson::Document& doc) {

	}
}