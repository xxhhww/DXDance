#pragma once

namespace Windows {
	enum class ECursorMode {
		NORMAL = 0x00034001,
		DISABLED = 0x00034003,
		HIDDEN = 0x00034002
	};

	enum class ECursorShape {
		ARROW = 0x00036001,
		IBEAM = 0x00036002,
		CROSSHAIR = 0x00036003,
		HAND = 0x00036004,
		HRESIZE = 0x00036005,
		VRESIZE = 0x00036006
	};
}