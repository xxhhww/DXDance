#include "Scene.h"

namespace Core {
	Actor* Scene::CreateActor(const std::string& name) {
		mActors.emplace_back(std::make_unique<Actor>(name));
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

	void Scene::SerializeJson(rapidjson::Document& doc) const {

	}

	void Scene::DeserializeJson(const rapidjson::Document& doc) {

	}
}