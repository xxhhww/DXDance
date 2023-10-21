#pragma once
#include "Renderer/VegetationPhysicalDataCache.h"

namespace Renderer {

	class VegetationVirtualTable {
	public:
		struct GrassClusterTableCell {
		public:
			Math::Vector4 currGrassClusterRect;
			VegetationDataCache::GrassClusterCache::Node* cahce{ nullptr };
		};

	public:
		VegetationVirtualTable();
		~VegetationVirtualTable();

	private:
		std::vector<std::vector<GrassClusterTableCell>> mGrassClusterTable;
	};

}