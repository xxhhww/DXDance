#pragma once
#include "UI/IWidget.h"

namespace App {
	class ImnodesObject : public UI::IWidget {
	public:
		inline ImnodesObject(int id) : objectID(id) {}
		virtual ~ImnodesObject() = default;
	public:
		int objectID{ -1 };
	};
}