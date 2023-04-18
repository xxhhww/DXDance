#pragma once
#include "IPanel.h"
#include "Setting.h"
#include "Math/Vector.h"

namespace UI {
	class IPanelTransformable : public IPanel {
	public:
		IPanelTransformable
		(
			const Math::Vector2& defaultPosition = Math::Vector2(-1.0f, -1.0f),
			const Math::Vector2& defaultSize = Math::Vector2(-1.f, -1.f),
			HorizontalAlignment defaultHorizontalAlignment = HorizontalAlignment::LEFT,
			VerticalAlignment defaultVerticalAlignment = VerticalAlignment::TOP,
			bool ignoreConfigFile = false
		);
		virtual ~IPanelTransformable() = default;

		void SetPosition(const Math::Vector2& position);
		void SetSize(const Math::Vector2& size);
		void SetAlignment(HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAligment);

		inline const auto& GetPosition() const { return mPosition; }
		inline const auto& GetSize() const { return mSize; }
		inline auto GetHorizontalAlignment() const { return mHorizontalAlignment; }
		inline auto GetVerticalAlignment() const { return mVerticalAlignment; }

	protected:
		void Update();
		virtual void _Draw_Internal_Impl() = 0;

	private:
		Math::Vector2 CalculatePositionAlignmentOffset(bool useDefault = false);

		void UpdatePosition();
		void UpdateSize();
		void CopyImGuiPosition();
		void CopyImGuiSize();

	public:
		bool autoSize{ true };

	protected:
		Math::Vector2 mDefaultPosition;
		Math::Vector2 mDefaultSize;
		HorizontalAlignment mDefaultHorizontalAlignment;
		VerticalAlignment mDefaultVerticalAlignment;
		bool mIgnoreConfigFile{ false };

		Math::Vector2 mPosition{ 0.0f, 0.0f };
		Math::Vector2 mSize{ 0.0f, 0.0f };

		bool mPositionChanged{ false };
		bool mSizeChanged{ false };

		HorizontalAlignment mHorizontalAlignment{ HorizontalAlignment::LEFT };
		VerticalAlignment mVerticalAlignment{ VerticalAlignment::TOP };

		bool mAlignmentChanged{ false };
	};
}