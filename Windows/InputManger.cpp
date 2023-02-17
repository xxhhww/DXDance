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
	}

	InputManger::~InputManger() {
		if (mWindow) {
			mWindow->keyPressedEvent.RemoveListener(mKeyPressedListenerID);
			mWindow->keyReleasedEvent.RemoveListener(mKeyReleasedListenerID);
			mWindow->mouseButtonPressedEvent.RemoveListener(mMouseButtonPressedListenerID);
			mWindow->mouseButtonReleasedEvent.RemoveListener(mMouseButtonReleasedListenerID);
		}
	}

	void InputManger::SetKeyRepeatDelay(float delay) {
		mKeyRepeatDelay = delay;
	}

	void InputManger::SetKeyRepeatRate(float rate) {
		mKeyRepeatRate = rate;
	}

	EKeyData& InputManger::GetKeyData(EKey key) {
		return mKeyDatas.at(static_cast<size_t>(key));
	}

	const EKeyData& InputManger::GetKeyData(EKey key) const {
		return mKeyDatas.at(static_cast<size_t>(key));
	}

	EMouseButtonState InputManger::GetMouseButtonState(EMouseButton button) const {
		return static_cast<EMouseButtonState>(mMouseButtonStates[static_cast<int>(button)]);
	}

	bool InputManger::IsKeyDown(EKey key) const {
		const EKeyData& keyData = GetKeyData(key);
		return keyData.down;
	}

	bool InputManger::IsKeyPressed(EKey key, bool autoRepeat) const {
		const EKeyData& keyData = GetKeyData(key);
		const float t = keyData.downDuration;
		if (t < 0.0f) {
			return false;
		}

		// 按键被按下
		bool pressed = (t == 0.0f);

		// 按键一直被按着
		if (!pressed && autoRepeat) {
			float repeatDelay, repeatRate;
			GetTypematicRepeatRate(&repeatDelay, &repeatRate);
			pressed = (t > repeatDelay) && GetKeyPressedAmount(key, repeatDelay, repeatRate) > 0;
		}

		return pressed;
	}

	bool InputManger::IsKeyReleased(EKey key) const {
		const EKeyData& keyData = GetKeyData(key);
		return !keyData.down && keyData.downDurationPrev >= 0.0f;
	}

	bool InputManger::IsMouseButtonPressed(EMouseButton button) const {
		return mMouseButtonStates[static_cast<int>(button)];
	}

	bool InputManger::IsMouseButtonReleased(EMouseButton button) const {
		return mMouseButtonStates[static_cast<int>(button)];
	}

	void InputManger::PreUpdate(float delta) {
		while (!mKeyEventQueue.empty()) {
			const EKeyEvent& event = mKeyEventQueue.front();
			
			EKeyData& keyData = GetKeyData(event.key);
			keyData.down = event.down;
			keyData.downDurationPrev = keyData.downDuration;
			keyData.downDuration = keyData.down ? (keyData.downDuration < 0.0f ? 0.0f : keyData.downDuration + delta) : -1.0f;

			mKeyEventQueue.pop();
		}
	}

	void InputManger::PostUpdate() {
		mMouseButtonStates.reset();
	}

	void InputManger::GetTypematicRepeatRate(float* repeatDelay, float* repeatRate) const {
		*repeatDelay = mKeyRepeatDelay;
		*repeatRate = mKeyRepeatRate;
	}

	int InputManger::GetKeyPressedAmount(EKey key, float repeatDelay, float repeatRate) const {
		const EKeyData& keyData = GetKeyData(key);
		const float t = keyData.downDuration;
		return CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, repeatDelay, repeatRate);
	}

	int InputManger::CalcTypematicRepeatAmount(float t0, float t1, float repeatDelay, float repeatRate) const {
		if (t1 == 0.0f)
			return 1;
		if (t0 >= t1)
			return 0;
		if (repeatRate <= 0.0f)
			return (t0 < repeatDelay) && (t1 >= repeatDelay);
		const int count_t0 = (t0 < repeatDelay) ? -1 : (int)((t0 - repeatDelay) / repeatRate);
		const int count_t1 = (t1 < repeatDelay) ? -1 : (int)((t1 - repeatDelay) / repeatRate);
		const int count = count_t1 - count_t0;
		return count;
	}

	void InputManger::OnKeyPressed(EKey key) {
		mKeyEventQueue.emplace(key, true);
	}

	void InputManger::OnKeyReleased(EKey key) {
		mKeyEventQueue.emplace(key, false);
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