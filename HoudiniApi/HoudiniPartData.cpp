#include "HoudiniApi/HoudiniPartData.h"
#include "HoudiniApi/HoudiniGeoNode.h"
#include "HoudiniApi/HoudiniTemplateUtility.h"
#include "Tools/Assert.h"
#include "Math/Vector.h"
#include "Math/Quaternion.h"

namespace Houdini {

	HoudiniPartData::HoudiniPartData(const HAPI_Session* session, HAPI_PartId hapiPartID, HAPI_NodeId hapiGeoID, HAPI_NodeId hapiObjectNodeID, HoudiniGeoNode* houdiniGeoNode,
		const HAPI_PartInfo& hapiPartInfo, bool bIsEditable, bool bIsObjectInstancer, bool bIsAttribInstancer) {
		partID = hapiPartID;
		geoID = hapiGeoID;
		objectNodeID = hapiObjectNodeID;
		geoNode = houdiniGeoNode;
		partType = hapiPartInfo.type;

		partName = HoudiniApiUtility::GetString(session, hapiPartInfo.nameSH);

		isPartInstanced = hapiPartInfo.isInstanced;
		partPointCount = hapiPartInfo.pointCount;
		isPartEditable = bIsEditable;
		meshVertexCount = hapiPartInfo.vertexCount;
		isAttribInstancer = bIsAttribInstancer;

		isObjectInstancer = bIsObjectInstancer;
		objectInstancesGenerated = false;

		volumeLayerName = "";
	}

	void HoudiniPartData::GeneratePartInstances(const HAPI_Session* session) {
		HAPI_PartInfo partInfo{};
		HAPI_Result hapiResult = FHoudiniApi::GetPartInfo(session, geoID, partID, &partInfo);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetPartInfo Failed");
		ASSERT_FORMAT(partInfo.instancedPartCount > 0, "Invalid instanced part count");

		// Get each instance's transform
		std::vector<HAPI_Transform> instanceTransforms(partInfo.instanceCount);
		HoudiniTemplateUtility::GetArray3ArgFunc<HAPI_RSTOrder, HAPI_PartId, HAPI_Transform> func1 = std::bind(FHoudiniApi::GetInstancerPartTransforms, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
		HoudiniTemplateUtility::GetArray3Arg(
			session, geoID, partID, HAPI_SRT,  func1,
			instanceTransforms.data(), 0, partInfo.instanceCount);

		// Get part IDs for the parts being instanced
		std::vector<HAPI_NodeId> instanceNodeIDs(partInfo.instancedPartCount);
		HoudiniTemplateUtility::GetArray2ArgFunc<HAPI_PartId, HAPI_NodeId> func2 = std::bind(FHoudiniApi::GetInstancedPartIds, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
		HoudiniTemplateUtility::GetArray2Arg(
			session, geoID, partID, func2, instanceNodeIDs.data(), 0, partInfo.instancedPartCount
		);

		// Get instance names if set
		/*
		string[] instancePrefixes = null;
		HAPI_AttributeInfo instancePrefixAttrInfo = new HAPI_AttributeInfo();
		HEU_GeneralUtility.GetAttributeInfo(session, _geoID, PartID, HEU_Defines.DEFAULT_INSTANCE_PREFIX_ATTR, ref instancePrefixAttrInfo);
		if (instancePrefixAttrInfo.exists)
		{
			instancePrefixes = HEU_GeneralUtility.GetAttributeStringData(session, _geoID, PartID, HEU_Defines.DEFAULT_INSTANCE_PREFIX_ATTR, ref instancePrefixAttrInfo);
		}
		*/

		int32_t numInstances = instanceNodeIDs.size();
		for (int32_t i = 0; i < numInstances; ++i) {
			HoudiniPartData* partData = geoNode->GetPartFromPartID(instanceNodeIDs[i]);
			ASSERT_FORMAT(partData != nullptr, "Part is missing");

			// If the part we're instancing is itself an instancer, make sure it has generated its instances
			if (partData->IsPartInstancer()) {
				ASSERT_FORMAT(false, "Just Failed");
			}

			HAPI_PartInfo instancePartInfo{};
			hapiResult = FHoudiniApi::GetPartInfo(session, geoID, instanceNodeIDs[i], &instancePartInfo);
			ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetPartInfo Failed");

			int32_t numTransforms = instanceTransforms.size();
			for (int32_t j = 0; j < numTransforms; ++j) {
				// Houdini use right-handed, convert to left-handed
				Math::Vector3 finalPosition{ 
					instanceTransforms[j].position[0], 
					instanceTransforms[j].position[1], 
					-instanceTransforms[j].position[2] 
				};
				
				Math::Quaternion finalQuaternion{ 
					-instanceTransforms[j].rotationQuaternion[0], 
					-instanceTransforms[j].rotationQuaternion[1],
					instanceTransforms[j].rotationQuaternion[2],
					instanceTransforms[j].rotationQuaternion[3] 
				};

				Math::Vector3 finalScaling{
					instanceTransforms[j].scale[0],
					instanceTransforms[j].scale[1],
					instanceTransforms[j].scale[2],
				};
			}
		}
	}

}