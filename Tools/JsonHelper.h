#pragma once
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace Tool {
	using JsonWriter = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
	using JsonReader = rapidjson::Value;
}