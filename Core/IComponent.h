#pragma once
#include "IInspectorItem.h"

namespace Core {

	class IComponent : public IInspectorItem {
	public:
		virtual ~IComponent() = default;

		void SerializeJson(rapidjson::Document& doc) const override {}

		void DeserializeJson(const rapidjson::Document& doc) override {}
	};
}