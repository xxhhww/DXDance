#include "IPanelTransformable.h"
#include "imgui.h"

namespace UI {
	IPanelTransformable::IPanelTransformable
	(
		const Math::Vector2& defaultPosition,
		const Math::Vector2& defaultSize,
		HorizontalAlignment defaultHorizontalAlignment,
		VerticalAlignment defaultVerticalAlignment,
		bool ignoreConfigFile
	)
	: mDefaultPosition(defaultPosition)
	, mDefaultSize(defaultSize)
	, mDefaultHorizontalAlignment(defaultHorizontalAlignment)
	, mDefaultVerticalAlignment(defaultVerticalAlignment)
	, mIgnoreConfigFile(ignoreConfigFile) {}

	void IPanelTransformable::SetPosition(const Math::Vector2& position) {
		mPosition = position;
		mPositionChanged = true;
	}

	void IPanelTransformable::SetSize(const Math::Vector2& size) {
		mSize = size;
		mSizeChanged = true;
	}

	void IPanelTransformable::SetAlignment(HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAligment) {
		mHorizontalAlignment = horizontalAlignment;
		mVerticalAlignment = verticalAligment;
		mAlignmentChanged = true;
	}

	void IPanelTransformable::Update() {
		CopyImGuiSize();
		CopyImGuiPosition();
	}

	Math::Vector2 IPanelTransformable::CalculatePositionAlignmentOffset(bool useDefault) {
		Math::Vector2 result(0.0f, 0.0f);

		switch (useDefault ? mDefaultHorizontalAlignment : mHorizontalAlignment) {
		case HorizontalAlignment::CENTER:
			result.x -= mSize.x / 2.0f;
			break;
		case HorizontalAlignment::RIGHT:
			result.x -= mSize.x;
			break;
		}

		switch (useDefault ? mDefaultVerticalAlignment : mVerticalAlignment) {
		case VerticalAlignment::MIDDLE:
			result.y -= mSize.y / 2.0f;
			break;
		case VerticalAlignment::BOTTOM:
			result.y -= mSize.y;
			break;
		}

		return result;
	}

	void IPanelTransformable::UpdatePosition() {
		if (mDefaultPosition.x != -1.0f && mDefaultPosition.y != -1.0f) {
			Math::Vector2 offsettedDefaultPos = mDefaultPosition + CalculatePositionAlignmentOffset(true);
			ImGui::SetWindowPos(ImVec2(offsettedDefaultPos.x, offsettedDefaultPos.y), mIgnoreConfigFile ? ImGuiCond_Once : ImGuiCond_FirstUseEver);
		}

		if (mPositionChanged || mAlignmentChanged) {
			Math::Vector2 offset = CalculatePositionAlignmentOffset(false);
			Math::Vector2 offsettedPos(mPosition.x + offset.x, mPosition.y + offset.y);
			ImGui::SetWindowPos(ImVec2(offsettedPos.x, offsettedPos.y), ImGuiCond_Always);
			mPositionChanged = false;
			mAlignmentChanged = false;
		}
	}

	void IPanelTransformable::UpdateSize() {
		if (mSizeChanged) {
			ImGui::SetWindowSize(ImVec2(mSize.x, mSize.y), ImGuiCond_Always);
			mSizeChanged = false;
		}
	}

	void IPanelTransformable::CopyImGuiPosition() {
		ImVec2 imguiPos = ImGui::GetWindowPos();
		mPosition = Math::Vector2(imguiPos.x, imguiPos.y);
	}

	void IPanelTransformable::CopyImGuiSize() {
		ImVec2 imguiSize = ImGui::GetWindowSize();
		mSize = Math::Vector2(imguiSize.x, imguiSize.y);
	}
}