#pragma once
#include "JsonHelper.h"
#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

#include <string>

namespace Tool {
	class SerializeHelper {
	public:
		// 序列化

		static void SerializeString(JsonWriter& writer, const char* key, const std::string& value);

		static void SerializeBool(JsonWriter& writer, const char* key, const bool& value);

		static void SerializeInt32(JsonWriter& writer, const char* key, const int32_t& value);

		static void SerializeInt64(JsonWriter& writer, const char* key, const int64_t& value);

		static void SerializeFloat(JsonWriter& writer, const char* key, const float& value);

		static void SerializeDouble(JsonWriter& writer, const char* key, const double& value);

		static void SerializeVector2(JsonWriter& writer, const char* key, const Math::Vector2& value);

		static void SerializeVector3(JsonWriter& writer, const char* key, const Math::Vector3& value);

		static void SerializeVector4(JsonWriter& writer, const char* key, const Math::Vector4& value);

		static void SerializeQuaternion(JsonWriter& writer, const char* key, const Math::Quaternion& value);

		static void SerializeMatrix(JsonWriter& writer, const char* key, const Math::Matrix4& value);


		// 反序列化

		static void DeserializeString(const JsonReader& reader, const char* key, std::string& value);

		static void DeserializeBool(const JsonReader& reader, const char* key, bool& value);

		static void DeserializeInt32(const JsonReader& reader, const char* key, int32_t& value);

		static void DeserializeInt64(const JsonReader& reader, const char* key, int64_t& value);

		static void DeserializeFloat(const JsonReader& reader, const char* key, float& value);

		static void DeserializeDouble(const JsonReader& reader, const char* key, double& value);

		static void DeserializeVector2(const JsonReader& reader, const char* key, Math::Vector2& value);

		static void DeserializeVector3(const JsonReader& reader, const char* key, Math::Vector3& value);

		static void DeserializeVector4(const JsonReader& reader, const char* key, Math::Vector4& value);

		static void DeserializeQuaternion(const JsonReader& reader, const char* key, Math::Quaternion& value);

		static void DeserializeMatrix(const JsonReader& reader, const char* key, Math::Matrix4& value);
	};
}