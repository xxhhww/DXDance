#pragma once
#include "Math/Vector.h"

namespace Game {

	struct GlobalSetting {
	public:
		bool isPaused{ true };	// 是否暂停
		Math::Vector3 playerPosition{ -64.0f, 0.0f, 0.0f };	// 玩家控制的物体位置
	};

}