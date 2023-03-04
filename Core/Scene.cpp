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

	void Scene::SerializeJson(Tool::JsonWriter& writer) const {
		writer.StartObject();

		writer.Key("ActorIncID");
		writer.Int64(mActorIncID);

		writer.Key("Actors");
		writer.StartArray();
		for (auto& item : mActors) {
			item->SerializeJson(writer);
		}
		writer.EndArray();

		writer.EndObject();
	}

	void Scene::DeserializeJson(const Tool::JsonReader& reader) {
		assert(reader.HasMember("Actors") && reader["Actors"].IsArray());

		const Tool::JsonReader& childReader = reader["Actors"];
		for (size_t i = 0; i < childReader.Size(); i++) {
			assert(childReader[i].IsObject());
			// 创建一个默认的Actor，然后序列化
			Actor* actor = CreateActor("");
			actor->DeserializeJson(childReader[i]);
		}

		// 设置父子层级
		for (auto& actor : mActors) {
			Actor* parent = FindActorByID(actor->GetParentID());
			if (parent != nullptr) {
				actor->AttachParent(*parent);
			}
		}

		assert(reader.HasMember("ActorIncID") && reader["ActorIncID"].IsInt64());
		mActorIncID = reader["ActorIncID"].GetInt64();
	}
}