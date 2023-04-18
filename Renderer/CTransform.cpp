#include "CTransform.h"

namespace Renderer {

	void Transform::SerializeJson(Tool::JsonWriter& writer) const {
		using namespace Tool;

		writer.StartObject();

		SerializeHelper::SerializeString(writer, "Typename", std::string(typeid(Transform).name()));

		SerializeHelper::SerializeVector3(writer, "WorldPosition", worldPosition);
		SerializeHelper::SerializeQuaternion(writer, "WorldRotation", worldRotation);
		SerializeHelper::SerializeVector3(writer, "WorldScale", worldScale);
		SerializeHelper::SerializeMatrix(writer, "WorldMatrix", worldMatrix);

		writer.EndObject();
	}

	void Transform::DeserializeJson(const Tool::JsonReader& reader) {
		using namespace Tool;

		// Typename”……œ≤„Ω‚Œˆ
		SerializeHelper::DeserializeVector3(reader, "WorldPosition", worldPosition);
		SerializeHelper::DeserializeQuaternion(reader, "WorldRotation", worldRotation);
		SerializeHelper::DeserializeVector3(reader, "WorldScale", worldScale);
		SerializeHelper::DeserializeMatrix(reader, "WorldMatrix", worldMatrix);
	}

}