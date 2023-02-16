#include "InputManger.h"

namespace Windows {
	InputManger::InputManger(Window* window)
	: mWindow(window) {
		// ×¢²á»Øµ÷
		mKeyPressedListenerID = 
			mWindow->keyPressedEvent.AddListener(std::bind(&InputManger::OnKeyPressed, this, std::placeholders::_1));
		mKeyReleasedListenerID = 
			mWindow->keyReleasedEvent.AddListener(std::bind(&InputManger::OnKeyReleased, this, std::placeholders::_1));
		mMouseButtonPressedListenerID = 
			mWindow->mouseButtonPressedEvent.AddListener(std::bind(&InputManger::OnMouseButtonPressed, this, std::placeholders::_1));
		mMouseButtonReleasedListenerID = 
			mWindow->mouseButtonReleasedEvent.AddListener(std::bind(&InputManger::OnMouseButtonReleased, this, std::placeholders::_1));
		mMouseMoveListenerID = 
			mWindow->mouseMoveEvent.AddListener(std::bind(&InputManger::OnMouseMove, this, std::placeholders::_1, std::placeholders::_2));
		mRawDeltaListenerID = 
			mWindow->rawDeltaEvent.AddListener(std::bind(&InputManger::OnRawDelta, this, std::placeholders::_1, std::placeholders::_2));
	}

	InputManger::~InputManger() {
		if (mWindow) {
			mWindow->keyPressedEvent.RemoveListener(mKeyPressedListenerID);
			mWindow->keyReleasedEvent.RemoveListener(mKeyReleasedListenerID);
			mWindow->mouseButtonPressedEvent.RemoveListener(mMouseButtonPressedListenerID);
			mWindow->mouseButtonReleasedEvent.RemoveListener(mMouseButtonReleasedListenerID);
		}
	}

	EKeyState InputManger::GetKeyState(EKey key) const {
		return static_cast<EKeyState>(mKeyStates[static_cast<int>(key)]);
	}

	EMouseButtonState InputManger::GetMouseButtonState(EMouseButton button) const {
		return static_cast<EMouseButtonState>(mMouseButtonStates[static_cast<int>(button)]);
	}

	bool InputManger::IsKeyPressed(EKey key) const {
		return mKeyStates[static_cast<int>(key)];
	}

	bool InputManger::IsKeyReleased(EKey key) const {
		return mKeyStates[static_cast<int>(key)];
	}

	bool InputManger::IsMouseButtonPressed(EMouseButton button) const {
		return mMouseButtonStates[static_cast<int>(button)];
	}

	bool InputManger::IsMouseButtonReleased(EMouseButton button) const {
		return mMouseButtonStates[static_cast<int>(button)];
	}

	void InputManger::ClearStates() {
		mKeyStates.reset();
		mMouseButtonStates.reset();
	}

	void InputManger::OnKeyPressed(EKey key) {
		mKeyStates[static_cast<int>(key)] = 1;
	}

	void InputManger::OnKeyReleased(EKey key) {
		mKeyStates[static_cast<int>(key)] = 0;
	}

	void InputManger::OnMouseButtonPressed(EMouseButton button) {
		mMouseButtonStates[static_cast<int>(button)] = true;
	}

	void InputManger::OnMouseButtonReleased(EMouseButton button) {
		mMouseButtonStates[static_cast<int>(button)] = false;
	}

	void InputManger::OnMouseMove(int16_t x, int16_t y) {
		mMousePosition = Math::Vector2{ (float)x, (float)y };
	}

	void InputManger::OnRawDelta(int32_t dx, int32_t dy) {
		mRawDelta = Math::Vector2{ (float)dx, (float)dy };
	}
}