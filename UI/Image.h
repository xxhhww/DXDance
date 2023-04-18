#pragma once
#include "IWidget.h"
#include "Math/Vector.h"

namespace UI {

	class Image : public IWidget {
	public:
		Image(uint64_t texID, const Math::Vector2& size);
		~Image() = default;

	protected:
		void _Draw_Internal_Impl() override;

	public:
		uint64_t textureID{ 0u };
		Math::Vector2 size{ 0.0f, 0.0f };
	};

}