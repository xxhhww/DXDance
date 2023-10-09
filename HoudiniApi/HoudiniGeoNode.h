#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include <string>

namespace Houdini {

	struct HoudiniObjectNode;
	/*
	* 
	*/
	struct HoudiniGeoNode {
	public:
		std::string  geoName;
		HAPI_GeoInfo geoInfo;
		HAPI_GeoType geoType;

		HoudiniObjectNode* containerObjectNode{ nullptr };
	};

}