#include "Scene.h"

namespace Core {
	Scene::Scene(IAssetManger<Scene>* manger)
	: mManger(manger) {}

	void Scene::Load(bool aSync) {
		std::ifstream inputStream;
		inputStream.open(mManger->GetRealPath(mPath));
		if (!inputStream.is_open()) {
			assert(false);
		}

		std::string jsonData((std::istreambuf_iterator<char>(inputStream)), std::istreambuf_iterator<char>());
		inputStream.close();

		rapidjson::Document doc;
		if (doc.Parse(jsonData.c_str()).HasParseError()) {
			assert(false);
		}

		const rapidjson::Value& rootObj = doc.GetObj();
		this->DeserializeJson(rootObj);
	}

	void Scene::Unload() {
		rapidjson::StringBuffer buf;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

		this->SerializeJson(writer);

		std::ofstream outputStream;
		outputStream.open(mManger->GetRealPath(mPath));
		if (!outputStream.is_open()) {
			assert(false);
		}

		outputStream << buf.GetString() << std::endl;
		outputStream.close();
	}

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