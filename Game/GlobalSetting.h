#pragma once
#include "Math/Vector.h"

namespace Game {

	struct GlobalSetting {
	public:
		bool isPaused{ true };	// �Ƿ���ͣ
		Math::Vector3 playerPosition{ -64.0f, 0.0f, 0.0f };	// ��ҿ��Ƶ�����λ��
	};

}