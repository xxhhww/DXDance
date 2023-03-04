#include "SerializeHepler.h"

namespace Tool {
	void SerializeHelper::SerializeString(JsonWriter& writer, const char* key, const std::string& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.String(value.c_str());
	}

	void SerializeHelper::SerializeBool(JsonWriter& writer, const char* key, const bool& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.Bool(value);
	}

	void SerializeHelper::SerializeInt32(JsonWriter& writer, const char* key, const int32_t& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.Int(value);
	}

	void SerializeHelper::SerializeInt64(JsonWriter& writer, const char* key, const int64_t& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.Int64(value);
	}

	void SerializeHelper::SerializeFloat(JsonWriter& writer, const char* key, const float& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.Double(value);
	}

	void SerializeHelper::SerializeDouble(JsonWriter& writer, const char* key, const double& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.Double(value);
	}

	void SerializeHelper::SerializeVector2(JsonWriter& writer, const char* key, const Math::Vector2& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.EndArray();
	}

	void SerializeHelper::SerializeVector3(JsonWriter& writer, const char* key, const Math::Vector3& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.Double(value.z);
		writer.EndArray();
	}

	void SerializeHelper::SerializeVector4(JsonWriter& writer, const char* key, const Math::Vector4& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.Double(value.z);
		writer.Double(value.w);
		writer.EndArray();
	}

	void SerializeHelper::SerializeQuaternion(JsonWriter& writer, const char* key, const Math::Quaternion& value) {
		if (key != nullptr) {
			writer.Key(key);
		}
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.Double(value.z);
		writer.Double(value.w);
		writer.EndArray();
	}

	void SerializeHelper::SerializeMatrix(JsonWriter& writer, const char* key, const Math::Matrix4& value) {
		writer.Key(key);
		writer.StartArray();
		SerializeVector4(writer, nullptr, value.x());
		SerializeVector4(writer, nullptr, value.y());
		SerializeVector4(writer, nullptr, value.z());
		SerializeVector4(writer, nullptr, value.w());
		writer.EndArray();
	}


	void SerializeHelper::DeserializeString(const JsonReader& reader, const char* key, std::string& value) {
		if (key == nullptr) {
			assert(reader.IsString());
			value = std::string(reader.GetString());;
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsString());
		value = std::string(reader[key].GetString());
	}

	void SerializeHelper::DeserializeBool(const JsonReader& reader, const char* key, bool& value) {
		if (key == nullptr) {
			assert(reader.IsBool());
			value = reader.GetBool();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsBool());
		value = reader[key].GetBool();
	}

	void SerializeHelper::DeserializeInt32(const JsonReader& reader, const char* key, int32_t& value) {
		if (key == nullptr) {
			assert(reader.IsInt());
			value = reader.GetInt();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsInt());
		value = reader[key].GetInt();
	}

	void SerializeHelper::DeserializeInt64(const JsonReader& reader, const char* key, int64_t& value) {
		if (key == nullptr) {
			assert(reader.IsInt64());
			value = reader.GetInt64();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsInt64());
		value = reader[key].GetInt64();
	}

	void SerializeHelper::DeserializeFloat(const JsonReader& reader, const char* key, float& value) {
		if (key == nullptr) {
			assert(reader.IsDouble());
			value = reader.GetFloat();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsDouble());
		value = reader[key].GetFloat();
	}

	void SerializeHelper::DeserializeDouble(const JsonReader& reader, const char* key, double& value) {
		if (key == nullptr) {
			assert(reader.IsDouble());
			value = reader.GetDouble();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsDouble());
		value = reader[key].GetDouble();
	}

	void SerializeHelper::DeserializeVector2(const JsonReader& reader, const char* key, Math::Vector2& value) {
		if (key == nullptr) {
			assert(reader.IsArray());
			value.x = reader[0].GetFloat();
			value.y = reader[1].GetFloat();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsArray());
		const JsonReader& ary = reader[key];
		value.x = ary[0].GetFloat();
		value.y = ary[1].GetFloat();
	}

	void SerializeHelper::DeserializeVector3(const JsonReader& reader, const char* key, Math::Vector3& value) {
		if (key == nullptr) {
			assert(reader.IsArray());
			value.x = reader[0].GetFloat();
			value.y = reader[1].GetFloat();
			value.z = reader[2].GetFloat();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsArray());
		const JsonReader& ary = reader[key];
		value.x = ary[0].GetFloat();
		value.y = ary[1].GetFloat();
		value.z = ary[2].GetFloat();
	}

	void SerializeHelper::DeserializeVector4(const JsonReader& reader, const char* key, Math::Vector4& value) {
		if (key == nullptr) {
			assert(reader.IsArray());
			value.x = reader[0].GetFloat();
			value.y = reader[1].GetFloat();
			value.z = reader[2].GetFloat();
			value.w = reader[3].GetFloat();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsArray());
		const JsonReader& ary = reader[key];
		value.x = ary[0].GetFloat();
		value.y = ary[1].GetFloat();
		value.z = ary[2].GetFloat();
		value.w = ary[3].GetFloat();
	}

	void SerializeHelper::DeserializeQuaternion(const JsonReader& reader, const char* key, Math::Quaternion& value) {
		if (key == nullptr) {
			assert(reader.IsArray());
			value.x = reader[0].GetFloat();
			value.y = reader[1].GetFloat();
			value.z = reader[2].GetFloat();
			value.w = reader[3].GetFloat();
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsArray());
		const JsonReader& ary = reader[key];
		value.x = ary[0].GetFloat();
		value.y = ary[1].GetFloat();
		value.z = ary[2].GetFloat();
		value.w = ary[3].GetFloat();
	}

	void SerializeHelper::DeserializeMatrix(const JsonReader& reader, const char* key, Math::Matrix4& value) {
		if (key == nullptr) {
			assert(reader.IsArray());
			DeserializeVector4(reader[0], nullptr, value.x());
			DeserializeVector4(reader[1], nullptr, value.y());
			DeserializeVector4(reader[2], nullptr, value.z());
			DeserializeVector4(reader[3], nullptr, value.w());
			return;
		}

		assert(reader.HasMember(key) && reader[key].IsArray());
		const JsonReader& ary = reader[key];
		DeserializeVector4(ary[0], nullptr, value.x());
		DeserializeVector4(ary[1], nullptr, value.y());
		DeserializeVector4(ary[2], nullptr, value.z());
		DeserializeVector4(ary[3], nullptr, value.w());
	}
}