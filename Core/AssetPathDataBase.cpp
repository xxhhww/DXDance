#include "AssetPathDataBase.h"
#include "Tools/JsonHelper.h"

#include <fstream>

namespace Core {
	AssetPathDataBase::AssetPathDataBase(const std::string& tablePath)
	: mTablePath(tablePath) {
		LoadFromDisk();
	}

	AssetPathDataBase::~AssetPathDataBase() {
		SaveToDisk();
	}

	std::string AssetPathDataBase::GetPath(int64_t id) {
		auto it = mPathMap.find(id);

		if (it == mPathMap.end()) {
			return {};
		}

		return mPathMap.at(id);
	}

	int64_t AssetPathDataBase::GetID(const std::string& path) {
		for (const auto& pair : mPathMap) {
			if (pair.second == path) {
				return pair.first;
			}
		}
		return -1;
	}

	void AssetPathDataBase::PathChanged(int64_t id, const std::string& newPath) {
		auto it = mPathMap.find(id);

		if (it == mPathMap.end()) {
			mPathMap[id] = newPath;
			return;
		}

		if (mPathMap.at(id) == newPath) {
			return;
		}

		mPathMap.at(id) = newPath;

		SaveToDisk();
	}

	void AssetPathDataBase::LoadFromDisk() {
		std::ifstream inputStream;
		inputStream.open(mTablePath);
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
		for (auto it = rootObj.MemberBegin(); it != rootObj.MemberEnd(); ++it) {
			const rapidjson::Value& key = it->name;
			const rapidjson::Value& value = it->value;

			int64_t id = std::atoll(key.GetString());

			mPathMap[id] = value.GetString();
		}
	}

	void AssetPathDataBase::SaveToDisk() {
		rapidjson::StringBuffer buf;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

		writer.StartObject();

		for (const auto& pair : mPathMap) {
			writer.Key(std::to_string(pair.first).c_str());
			writer.String(pair.second.c_str());
		}

		writer.EndObject();

		std::ofstream outputStream;
		outputStream.open(mTablePath);
		if (!outputStream.is_open()) {
			assert(false);
		}
		outputStream << buf.GetString() << std::endl;
		outputStream.close();
	}
}