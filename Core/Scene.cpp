#include "Scene.h"

namespace Core {
	Actor* Scene::CreateActor(const std::string& name) {
		mActors.emplace_back(std::make_unique<Actor>(++mActorIncID, name));
		return mActors.back().get();
	}

	Actor* Scene::FindActorByID(int32_t id) {
		auto it = std::find_if(mActors.begin(), mActors.end(),
			[&](std::unique_ptr<Actor>& item) {
				return item->GetID() == id;
			});

		return it == mActors.end() ? nullptr : (*it).get();
	}

	Actor* Scene::FindActorByName(const std::string& name) {
		auto it = std::find_if(mActors.begin(), mActors.end(),
			[&](std::unique_ptr<Actor>& item) {
				return item->GetName() == name;
			});

		return it == mActors.end() ? nullptr : (*it).get();
	}

	void Scene::SerializeBinary(Tool::OutputMemoryStream& blob) const {
		blob.Write(mActorIncID);
		blob.Write(mActors.size());
		for (const auto& actor : mActors) {
			actor->SerializeBinary(blob);
		}
	}

	void Scene::DeserializeBinary(Tool::InputMemoryStream& blob) {
		blob.Read(mActorIncID);

		size_t actorSize{ 0u };
		blob.Read(actorSize);
		
		// ��ȡ����Actor������
		for (size_t i = 0; i < actorSize; i++) {
			Actor* actor = CreateActor("");
			actor->DeserializeBinary(blob);
			mActorIncID = mActorIncID >= actor->GetID() ? mActorIncID : actor->GetID();
		}
		++mActorIncID;

		// ����Actor�Ĳ㼶��ϵ
		for (auto& actor : mActors) {
			int64_t parentID = actor->GetParentID();
			Actor* parent = FindActorByID(parentID);
			actor->AttachParent(*parent);
		}
	}

	void Scene::SerializeJson(rapidjson::Document& doc) const {

	}

	void Scene::DeserializeJson(const rapidjson::Document& doc) {

	}
}