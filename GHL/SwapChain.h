#pragma once
#include "Display.h"


namespace GHL {

	enum class BackBuffStrategy : uint8_t {
		Double = 2, Triple = 3
	};

	class SwapChain {
	public:
		SwapChain(
			const Display* display
		);
		
	private:

	};

}