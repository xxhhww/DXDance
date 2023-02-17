#pragma once

namespace Windows {
	/*
	enum class EKeyState : int {
		KEY_UP = 0,
		KEY_DOWN = 1
	};
	*/

	/*
	* 按钮数据
	*/
	struct EKeyData {
		bool	down{ false };				// True for if key is down
		float	downDuration{ -1.0f };		// Duration the key has been down (<0.0f: not pressed, 0.0f: just pressed, >0.0f: time held)
		float	downDurationPrev{ -1.0f };	// Last frame duration the key has been down
	};

	/*
	* 按钮事件
	*/
	struct EKeyEvent {
		inline EKeyEvent(EKey key, bool down) : key(key), down(down) {}

		EKey	key;
		bool	down;
	};
}