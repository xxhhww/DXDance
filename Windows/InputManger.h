#pragma once
#include "Window.h"
#include "EkeyState.h"
#include "EMouseButtonState.h"
#include "Math/Vector.h"
#include <bitset>

namespace Windows {
	class InputManger {
	public:
		InputManger(Window* window);
		~InputManger();

		EKeyState GetKeyState(EKey key) const;
		EMouseButtonState GetMouseButtonState(EMouseButton button) const;

		bool IsKeyPressed(EKey key) const;
		bool IsKeyReleased(EKey key) const;
		bool IsMouseButtonPressed(EMouseButton button) const;
		bool IsMouseButtonReleased(EMouseButton button) const;

		void ClearStates();
	private:
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

		std::bitset<128> mKeyStates;
		std::bitset<3>	 mMouseButtonStates;
		
		// std::unordered_map<EKey, EKeyState> mKeyStates;
		// std::unordered_map<EMouseButton, EMouseButtonState> mMouseButtonStates;
		// 鼠标位置
		Math::Vector2 mMousePosition;
		// 鼠标位移增量(dx, dy)
		Math::Vector2 mRawDelta;
	};
}