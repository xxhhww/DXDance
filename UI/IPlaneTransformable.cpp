#include "IPlaneTransformable.h"
#include "imgui.h"

namespace UI {
	/*
* Create a APanelTransformable
*/
	APanelTransformable::APanelTransformable
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

	/*
	* Defines the position of the panel
	*/
	void APanelTransformable::SetPosition(const Math::Vector2& position) {
		mPosition = position;
		mPositionChanged = true;
	}

	/*
	* Defines the size of the panel
	*/
	void APanelTransformable::SetSize(const Math::Vector2& size) {
		mSize = size;
		mSizeChanged = true;
	}

	/*
	* Defines the alignment of the panel
	*/
	void APanelTransformable::SetAlignment(HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAligment) {
		mHorizontalAlignment = horizontalAlignment;
		mVerticalAlignment = verticalAligment;
		mAlignmentChanged = true;
	}

	void APanelTransformable::Update() {

	}

	Math::Vector2 APanelTransformable::CalculatePositionAlignmentOffset(bool useDefault) {
		Math::Vector2 result(0.0f, 0.0f);

		switch (useDefault ? mDefaultHorizontalAlignment : mHorizontalAlignment)
		{
		case HorizontalAlignment::CENTER:
			result.x -= mSize.x / 2.0f;
			break;
		case HorizontalAlignment::RIGHT:
			result.x -= mSize.x;
			break;
		}

		switch (useDefault ? mDefaultVerticalAlignment : mVerticalAlignment)
		{
		case VerticalAlignment::MIDDLE:
			result.y -= mSize.y / 2.0f;
			break;
		case VerticalAlignment::BOTTOM:
			result.y -= mSize.y;
			break;
		}

		return result;
	}

	void APanelTransformable::UpdatePosition() {
		if (mDefaultPosition.x != -1.0f && mDefaultPosition.y != 1.0f) {
			Math::Vector2 offsettedDefaultPos = mDefaultPosition + CalculatePositionAlignmentOffset(true);
			ImGui::SetWindowPos(ImVec2(offsettedDefaultPos.x, offsettedDefaultPos.y), mIgnoreConfigFile ? ImGuiCond_Once : ImGuiCond_FirstUseEver);
		}

		if (mPositionChanged || mAlignmentChanged)
		{
			Math::Vector2 offset = CalculatePositionAlignmentOffset(false);
			Math::Vector2 offsettedPos(mPosition.x + offset.x, mPosition.y + offset.y);
			ImGui::SetWindowPos(ImVec2(offsettedPos.x, offsettedPos.y), ImGuiCond_Always);
			mPositionChanged = false;
			mAlignmentChanged = false;
		}
	}

	void APanelTransformable::UpdateSize() {

	}

	void APanelTransformable::CopyImGuiPosition() {

	}

	void APanelTransformable::CopyImGuiSize() {

	}
}