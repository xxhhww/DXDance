#include "HoudiniApi/HoudiniGeoNode.h"
#include "HoudiniApi/HoudiniObjectNode.h"
#include "HoudiniApi/HoudiniAsset.h"
#include "Tools/Assert.h"

namespace Houdini {

	HoudiniGeoNode::HoudiniGeoNode(const HAPI_Session* session, HoudiniObjectNode* objectNode, HAPI_GeoInfo& hapiGeoInfo) 
	: containerObjectNode(objectNode) 
	, geoInfo(hapiGeoInfo) {
		geoName = HoudiniApiUtility::GetString(session, geoInfo.nameSH);

		bool bObjectInstancer = containerObjectNode->IsInstancer();

		if (!geoInfo.isDisplayGeo && !containerObjectNode->parentAsset->useOutputNodes) {
			if (containerObjectNode->parentAsset->ignoreNonDisplayNodes) {
				return;
			}
			else if (!geoInfo.isEditable || 
					(geoInfo.type != HAPI_GEOTYPE_DEFAULT
					&& geoInfo.type != HAPI_GEOTYPE_INTERMEDIATE
					&& geoInfo.type != HAPI_GEOTYPE_CURVE)) {
				return;
			}
		}

		if (IsGeoCurveType()) {
			// ProcessGeoCurve(session);
		}
		else {
			int numParts = geoInfo.partCount;

			for (int i = 0; i < numParts; ++i) {
				HAPI_PartInfo partInfo{};
				HAPI_Result hapiResult = FHoudiniApi::GetPartInfo(session, geoInfo.nodeId, i, &partInfo);
				ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetPartInfo Failed");

				ProcessPart(session, i, partInfo);
			}
		}
	}

	void HoudiniGeoNode::ProcessPart(const HAPI_Session* session, int partID, const HAPI_PartInfo& partInfo) {
		HoudiniAsset* parentAsset = containerObjectNode->parentAsset;
		bool bResult = true;

		bool isPartEditable = IsIntermediateOrEditable();
		bool isAttribInstancer = false;

		if (IsGeoInputType()) {
			/*
			// Setup for input node to accept inputs
			if (_inputNode == null) {
				string partName = HEU_SessionManager.GetString(partInfo.nameSH, session);
				_inputNode = HEU_InputNode.CreateSetupInput(GeoID, 0, partName, partName, HEU_InputNode.InputNodeType.NODE, ParentAsset);
				if (_inputNode != null)
				{
					ParentAsset.AddInputNode(_inputNode);
				}
			}

			if (HEU_HAPIUtility.IsSupportedPolygonType(partInfo.type) && partInfo.vertexCount == 0) {
				// No geometry for input asset

				if (partData != null)
				{
					// Clean up existing part
					HEU_PartData.DestroyPart(partData);
					partData = null;
				}

				// No need to process further since we don't have geometry
				return;
			}
			*/
		}
		else {
			// Preliminary check for attribute instancing (mesh type with no verts but has points with instances)
			if (HoudiniApiUtility::IsSupportedPolygonType(partInfo.type) && partInfo.vertexCount == 0 && partInfo.pointCount > 0) {
				if (HoudiniApiUtility::HasValidInstanceAttribute(session, geoInfo.nodeId, partID, "unity_instance")) {
					isAttribInstancer = true;
				}
				else if (HoudiniApiUtility::HasValidInstanceAttribute(session, geoInfo.nodeId, partID, "unity_hf_treeinstance_prototypeindex")) {
					isAttribInstancer = true;
				}
			}
		}


		if (partInfo.type == HAPI_PARTTYPE_INVALID) {
			ASSERT_FORMAT(false, "partInfo invalid");
		}
		else if (partInfo.type < HAPI_PARTTYPE_MAX) {
			// Process the part based on type. Keep or ignore.

			// We treat parts of type curve as curves, along with geo nodes that are editable and type curves
			if (partInfo.type == HAPI_PARTTYPE_CURVE) {
				/*
				partData.Initialize(session, partID, GeoID, _containerObjectNode.ObjectID, this, ref partInfo,
					HEU_PartData.PartOutputType.CURVE, isPartEditable, _containerObjectNode.IsInstancer(), false);
				SetupGameObjectAndTransform(partData, parentAsset);
				partData.ProcessCurvePart(session);
				*/
			}
			else if (partInfo.type == HAPI_PARTTYPE_VOLUME) {
				// We only process "height" volume parts. Other volume parts are ignored for now.
				HAPI_VolumeInfo volumeInfo{};
				HAPI_Result hapiResult = FHoudiniApi::GetVolumeInfo(session, geoInfo.nodeId, partID, &volumeInfo);
				ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetVolumeInfo Failed");

				if (IsDisplayable() && !IsIntermediateOrEditable()) {
					std::unique_ptr<HoudiniPartData> partData = std::make_unique<HoudiniPartData>(session, partID, geoInfo.nodeId, containerObjectNode->objectInfo.nodeId,
						this, partInfo, isPartEditable, containerObjectNode->IsInstancer(), false);
					partDatas.emplace_back(std::move(partData));
				}
			}
			else if (partInfo.type == HAPI_PARTTYPE_INSTANCER || isAttribInstancer) {
				std::unique_ptr<HoudiniPartData> partData = std::make_unique<HoudiniPartData>(session, partID, geoInfo.nodeId, containerObjectNode->objectInfo.nodeId, 
					this, partInfo, isPartEditable, containerObjectNode->IsInstancer(), isAttribInstancer);
				partDatas.emplace_back(std::move(partData));
			}
			else if (HoudiniApiUtility::IsSupportedPolygonType(partInfo.type)) {
				std::unique_ptr<HoudiniPartData> partData = std::make_unique<HoudiniPartData>(session, partID, geoInfo.nodeId, containerObjectNode->objectInfo.nodeId,
					this, partInfo, isPartEditable, containerObjectNode->IsInstancer(), false);
				partDatas.emplace_back(std::move(partData));
			}
			else {
				ASSERT_FORMAT(false, "Unsupported part type");
			}
		}

	}

	void HoudiniGeoNode::GeneratePartInstances(const HAPI_Session* session) {
		for (auto& partData : partDatas) {
			if (partData->IsPartInstancer()) {
				partData->GeneratePartInstances(session);
			}
		}
	}

	HoudiniPartData* HoudiniGeoNode::GetPartFromPartID(HAPI_NodeId partID) {
		for (auto& partData : partDatas) {
			if (partData->partID == partID) {
				return partData.get();
			}
		}

		return nullptr;
	}

	bool HoudiniGeoNode::HasAttribInstancer() const {
		for (const auto& partData : partDatas) {
			if (partData->IsAttribInstancer()) {
				return true;
			}
		}
		return false;
	}

	bool HoudiniGeoNode::IsVisible() const { 
		return containerObjectNode->IsVisible(); 
	}

}