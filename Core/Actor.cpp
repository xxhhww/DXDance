#include "Actor.h"

#include "FooComponent.h"

#include "Renderer/CTransform.h"

namespace Core {
	Actor::Actor(int64_t actorID, const std::string& name) 
	: mActorID(actorID)
	, mName(name) 
	, mEntity(ECS::Entity::Create<Renderer::Transform>()) {
		Actor::ActorCreatedEvent.Invoke(this);
	}

	Actor::~Actor() {
		ECS::Entity::Delete(mEntity);
		Actor::ActorDestoryedEvent.Invoke(this);
	}

	void Actor::AttachParent(Actor* parent) {
		if (mParent == parent) {
			return;
		}

		// Attachʵ��
		mEntity.AttachParent(parent->mEntity);

		// AttachActor
		mParentID = parent->GetID();
		mParent = parent;
		parent->mChilds.push_back(this);

		ActorAttachEvent.Invoke(this, parent);
	}

	void Actor::DetachParent() {
		if (mParentID == -1) {
			return;
		}

		// Detachʵ��
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

	void Actor::SerializeJson(Tool::JsonWriter& writer) const {
		using namespace Tool;

		writer.StartObject();

		SerializeHelper::SerializeInt64(writer, "ID", mActorID);
		SerializeHelper::SerializeString(writer, "Name", mName);
		SerializeHelper::SerializeBool(writer, "Active", mActive);
		SerializeHelper::SerializeBool(writer, "Destoryed", mDestoryed);
		SerializeHelper::SerializeInt64(writer, "ParentID", mParentID);

		writer.Key("Components");
		writer.StartArray();
		mEntity.ForeachComp([&](ECS::IComponent* comp) {
			comp->SerializeJson(writer);
		});
		writer.EndArray();

		writer.EndObject();
	}

	void Actor::DeserializeJson(const Tool::JsonReader& reader) {
		using namespace Tool;

		SerializeHelper::DeserializeInt64(reader, "ID", mActorID);
		SerializeHelper::DeserializeString(reader, "Name", mName);
		SerializeHelper::DeserializeBool(reader, "Active", mActive);
		SerializeHelper::DeserializeBool(reader, "Destoryed", mDestoryed);
		SerializeHelper::DeserializeInt64(reader, "ParentID", mParentID);

		assert(reader.HasMember("Components") && reader["Components"].IsArray());
		const Tool::JsonReader& ary = reader["Components"];
		for (size_t i = 0; i < ary.Size(); i++) {
			std::string compname;
			SerializeHelper::DeserializeString(ary[i], "Typename", compname);
			if		(compname == typeid(Renderer::Transform).name())		{ GetComponent<Renderer::Transform>().DeserializeJson(ary[i]); }
			else if (compname == typeid(FooComponent).name())	{ AddComponent<FooComponent>().DeserializeJson(ary[i]); }
			else { assert(false); }
		}
	}
}