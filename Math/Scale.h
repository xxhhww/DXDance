#pragma once

namespace Math {

	/*
	* Ëõ·Å
	*/
	enum class Scale {
		ONE  = 0x00,
		HALF = 0x01,
		QUARTER = 0x02,
		EIGHTH  = 0x03
	};

	float Convert(const Scale& scale) {
		switch (scale) {
		case Scale::ONE:
			return 1.0f;
		case Scale::HALF:
			return 0.5f;
		case Scale::QUARTER:
			return 0.25f;
		case Scale::EIGHTH:
			return 0.125f;
		}
	}

}