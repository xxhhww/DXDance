#pragma once
#include "GHL/pbh.h"
#include <unordered_map>
#include <unordered_set>

namespace Renderer {

	class RenderGraphResource;

	class RendedrGraphResourceTracker {
	public:
		struct SubresourceTracker {
		public:
			uint32_t subresourceIndex{ 0u };
			GHL::EResourceState subresourceStates{ GHL::EResourceState::Common };
			
		};

	private:


	};

}