#include "InputManger.h"

namespace Windows {
	InputManger::InputManger(Window* window)
	: mWindow(window) {
		// 注册回调
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
		// 初始化按键与鼠标
		for (const auto& pair : sEKeyMap) {
			mKeyStates[pair.second] = EKeyState::KEY_UP;
		}
		for (const auto& pair : sEMouseButtonMap) {
			mMouseButtonStates[pair.second] = EMouseButtonState::MOUSE_UP;
		}
	}

	InputManger::~InputManger() {
		if (mWindow) {
			mWindow->keyPressedEvent.RemoveListener(mKeyPressedListenerID);
			mWindow->keyReleasedEvent.RemoveListener(mKeyReleasedListenerID);
			mWindow->mouseButtonPressedEvent.RemoveListener(mMouseButtonPressedListenerID);
			mWindow->mouseButtonReleasedEvent.RemoveListener(mMouseButtonReleasedListenerID);
		}
		mKeyStates.clear();
		mMouseButtonStates.clear();
	}

	EKeyState InputManger::GetKeyState(EKey key) const {
		return mKeyStates.at(key);
	}

	EMouseButtonState InputManger::GetMouseButtonState(EMouseButton button) const {
		return mMouseButtonStates.at(button);
	}

	bool InputManger::IsKeyPressed(EKey key) const {
		return (mKeyStates.at(key) == EKeyState::KEY_DOWN);
	}

	bool InputManger::IsKeyReleased(EKey key) const {
		return (mKeyStates.at(key) == EKeyState::KEY_UP);
	}

	bool InputManger::IsMouseButtonPressed(EMouseButton button) const {
		return (mMouseButtonStates.at(button) == EMouseButtonState::MOUSE_DOWN);
	}

	bool InputManger::IsMouseButtonReleased(EMouseButton button) const {
		return (mMouseButtonStates.at(button) == EMouseButtonState::MOUSE_UP);
	}

	void InputManger::OnKeyPressed(EKey key) {
		mKeyStates[key] = EKeyState::KEY_DOWN;
	}

	void InputManger::OnKeyReleased(EKey key) {
		mKeyStates[key] = EKeyState::KEY_UP;
	}

	void InputManger::OnMouseButtonPressed(EMouseButton button) {
		mMouseButtonStates[button] = EMouseButtonState::MOUSE_DOWN;
	}

	void InputManger::OnMouseButtonReleased(EMouseButton button) {
		mMouseButtonStates[button] = EMouseButtonState::MOUSE_UP;
	}

	void InputManger::OnMouseMove(int16_t x, int16_t y) {
		mMousePosition = Math::Vector2{ (float)x, (float)y };
	}

	void InputManger::OnRawDelta(int32_t dx, int32_t dy) {
		mRawDelta = Math::Vector2{ (float)dx, (float)dy };
	}
}