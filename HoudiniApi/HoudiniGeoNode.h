#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include "HoudiniApi/HoudiniPartData.h"
#include <string>
#include <memory>

namespace Houdini {

	struct HoudiniObjectNode;
	/*
	* 
	*/
	struct HoudiniGeoNode {
	public:
		std::string  geoName;
		HAPI_GeoInfo geoInfo;

		HoudiniObjectNode* containerObjectNode{ nullptr };
		std::vector<std::unique_ptr<HoudiniPartData>> partDatas;

	public:
		HoudiniGeoNode(const HAPI_Session* session, HoudiniObjectNode* objectNode, HAPI_GeoInfo& hapiGeoInfo);

		void ProcessPart(const HAPI_Session* session, int partID, const HAPI_PartInfo& partInfo);

		void GeneratePartInstances(const HAPI_Session* session);

		HoudiniPartData* GetPartFromPartID(HAPI_NodeId partID);

		bool HasAttribInstancer() const;

		bool IsVisible() const;

		inline bool IsDisplayable() const  { return geoInfo.isDisplayGeo; }

		inline bool IsIntermediate() const { return (geoInfo.type == HAPI_GEOTYPE_INTERMEDIATE); }

		inline bool IsIntermediateOrEditable() const { return (geoInfo.type == HAPI_GEOTYPE_INTERMEDIATE || (geoInfo.type == HAPI_GEOTYPE_DEFAULT && geoInfo.isEditable)); }

		inline bool IsGeoInputType() const { return geoInfo.isEditable && geoInfo.type == HAPI_GEOTYPE_INPUT; }

		inline bool IsGeoCurveType() const { return geoInfo.type == HAPI_GEOTYPE_CURVE; }
	};

}