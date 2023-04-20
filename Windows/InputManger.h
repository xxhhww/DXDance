#pragma once
#include "Window.h"
#include "EkeyState.h"
#include "EMouseButtonState.h"
#include "Math/Vector.h"
#include <bitset>
#include <array>
#include <queue>

namespace Windows {
	class InputManger {
	public:
		InputManger(Window* window);
		~InputManger();

		void SetKeyRepeatDelay(float delay);
		void SetKeyRepeatRate(float rate);
		EKeyData& GetKeyData(EKey key);
		const EKeyData& GetKeyData(EKey key) const;
		EMouseButtonState GetMouseButtonState(EMouseButton button) const;

		bool IsKeyDown(EKey key) const;
		bool IsKeyPressed(EKey key, bool autoRepeat = true) const;
		bool IsKeyReleased(EKey key) const;
		bool IsMouseMove() const;
		bool IsMouseButtonPressed(EMouseButton button) const;
		bool IsMouseButtonReleased(EMouseButton button) const;

		Math::Vector2 GetMouseRawDelta() const;

		void PreUpdate(float delta);
		void PostUpdate();
	private:
		void GetTypematicRepeatRate(float* repeatDelay, float* repeatRate) const;
		int GetKeyPressedAmount(EKey key, float repeatDelay, float repeatRate) const;
		int CalcTypematicRepeatAmount(float t0, float t1, float repeatDelay, float repeatRate) const;

		void OnKeyPressed(EKey key);
		void OnKeyReleased(EKey key);
		void OnMouseButtonPressed(EMouseButton button);
		void OnMouseButtonReleased(EMouseButton button);
		void OnMouseMove(int16_t x, int16_t y);
		void OnRawDelta(int32_t dx, int32_t dy);
	private:
		Window* mWindow{ nullptr };
		
		Tool::ListenerID mKeyPressedListenerID;
		Tool::ListenerID mKeyReleasedListenerID;
		Tool::ListenerID mMouseButtonPressedListenerID;
		Tool::ListenerID mMouseButtonReleasedListenerID;
		Tool::ListenerID mMouseMoveListenerID;
		Tool::ListenerID mRawDeltaListenerID;

		float mDeltaTime{ 0.0f };
		float mKeyRepeatDelay{ 0.5f };
		float mKeyRepeatRate{ 0.1f };
		std::queue<EKeyEvent> mKeyEventQueue;
		std::array<EKeyData, static_cast<size_t>(EKey::KEY_COUNT)> mKeyDatas;

		std::bitset<3>	 mMouseButtonStates;
		// 鼠标位置
		Math::Vector2 mMousePosition;
		// 鼠标位移增量(dx, dy)
		Math::Vector2 mRawDelta;
	};
}