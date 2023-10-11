#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include <string>
#include <vector>

namespace Houdini {

	struct HoudiniGeoNode;

	struct HoudiniPartData {
	public:
		std::string partName;
		HAPI_PartId partID;
		HAPI_NodeId geoID;
		HAPI_NodeId objectNodeID;
		HoudiniGeoNode* geoNode{ nullptr };

		HAPI_PartType partType;
		bool	isPartInstanced;
		int32_t partPointCount;
		bool	isPartEditable;
		int32_t meshVertexCount;
		bool	isAttribInstancer;

		bool	isObjectInstancer;
		bool	objectInstancesGenerated{ false };

		// std::vector<HoudiniObjectInstanceInfo> objectInstanceInfos;

		std::string volumeLayerName;

	public:
		HoudiniPartData(const HAPI_Session* session, HAPI_PartId hapiPartID, HAPI_NodeId hapiGeoID, HAPI_NodeId hapiObjectNodeID, HoudiniGeoNode* houdiniGeoNode,
			const HAPI_PartInfo& hapiPartInfo, bool bIsEditable, bool bIsObjectInstancer, bool bIsAttribInstancer);

		void GeneratePartInstances(const HAPI_Session* session);

		inline bool IsPartInstancer() const { return partType == HAPI_PARTTYPE_INSTANCER; }

		inline bool IsAttribInstancer() const { return isAttribInstancer; }
	};

}