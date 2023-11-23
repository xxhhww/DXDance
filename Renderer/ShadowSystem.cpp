#include "Renderer/ShadowSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/LinearBufferAllocator.h"

#include "Tools/Assert.h"

namespace Renderer {

	void ShadowSystem::Initialize(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		mCascadeShadowLightCameraViewMatrixs.resize(smNumShadowCascades);
		mCascadeShadowLightCameraProjectMatrixs.resize(smNumShadowCascades);
		mCascadeShadowDepthTextures.resize(smNumShadowCascades);

		// ����CascadeShadowDepthTexture
		Renderer::TextureDesc _CascadeShadowTextureDesc{};
		_CascadeShadowTextureDesc.width = smCascadeShadowDepthTextureResolution;
		_CascadeShadowTextureDesc.height = smCascadeShadowDepthTextureResolution;
		_CascadeShadowTextureDesc.format = DXGI_FORMAT_R24G8_TYPELESS;
		_CascadeShadowTextureDesc.initialState = GHL::EResourceState::Common;
		_CascadeShadowTextureDesc.expectedState = GHL::EResourceState::DepthWrite | GHL::EResourceState::NonPixelShaderAccess;
		for (int32_t i = 0; i < mCascadeShadowDepthTextures.size(); i++) {
			mCascadeShadowDepthTextures[i] = resourceAllocator->Allocate(device, _CascadeShadowTextureDesc, descriptorAllocator, nullptr);
			renderGraph->ImportResource("CascadeShadowDepthTexture:" + std::to_string(i), mCascadeShadowDepthTextures[i]);
			resourceStateTracker->StartTracking(mCascadeShadowDepthTextures[i]);
		}
	}

	void ShadowSystem::AddPass(RenderEngine* renderEngine) {
		auto* renderGraph = renderEngine->mRenderGraph.get();

		auto& finalOutputDesc =
			renderGraph->GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph->AddPass(
			"GBufferShadowPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;


			}
			);
	}

	void ShadowSystem::Update(RenderEngine* renderEngine) {
		ASSERT_FORMAT(renderEngine->mPipelineResourceStorage->rootLightDataPerFrame.size() > 0, "SunLight Missing");
		UpdateCascadeShadowLightCameraMatrix(renderEngine->mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera, renderEngine->mPipelineResourceStorage->rootLightDataPerFrame[0]);
	}

	void ShadowSystem::UpdateCascadeShadowLightCameraMatrix(const GPUCamera& gpuCamera, const GPULight& sunLight) {
		// ̫���ⷽ����ViewMatrix
		Math::Vector3 sunDirection{ sunLight.position.x, sunLight.position.y, sunLight.position.z };
		Math::Matrix4 sunViewMatrix = DirectX::XMMatrixLookToLH(Math::Vector3{ 0.0f, 0.0f, 0.0f }, sunDirection, Math::Vector3{ 0.0f, 1.0f, 0.0f });
		Math::Matrix4 invSunViewMatrix = sunViewMatrix.Inverse();

		for (int32_t i = 0; i < smNumShadowCascades; i++) {
			// ��ȡ��ǰ��Ⱦ����£���ǰ�����㼶��ƽ��ͷ��İ˸�����
			FrustumCorners currFrustumCornersInWorldSpace;
			SpawnWorldSpaceFrustumCornersByIndex(i, gpuCamera, currFrustumCornersInWorldSpace);

			auto getFrustumBoundingBox = [](const FrustumCorners& currFrustumCorners) {
				Math::BoundingBox currBoundingBox;

				std::vector<float> cornerXs = {
					currFrustumCorners.nearCorners[0].x, currFrustumCorners.nearCorners[1].x, currFrustumCorners.nearCorners[2].x, currFrustumCorners.nearCorners[3].x,
					currFrustumCorners.farCorners[0].x, currFrustumCorners.farCorners[1].x, currFrustumCorners.farCorners[2].x, currFrustumCorners.farCorners[3].x,
				};
				std::vector<float> cornerYs = {
					currFrustumCorners.nearCorners[0].y, currFrustumCorners.nearCorners[1].y, currFrustumCorners.nearCorners[2].y, currFrustumCorners.nearCorners[3].y,
					currFrustumCorners.farCorners[0].y, currFrustumCorners.farCorners[1].y, currFrustumCorners.farCorners[2].y, currFrustumCorners.farCorners[3].y,
				};
				std::vector<float> cornerZs = {
					currFrustumCorners.nearCorners[0].z, currFrustumCorners.nearCorners[1].z, currFrustumCorners.nearCorners[2].z, currFrustumCorners.nearCorners[3].z,
					currFrustumCorners.farCorners[0].z, currFrustumCorners.farCorners[1].z, currFrustumCorners.farCorners[2].z, currFrustumCorners.farCorners[3].z,
				};

				float minX = *std::min_element(cornerXs.begin(), cornerXs.end());
				float maxX = *std::max_element(cornerXs.begin(), cornerXs.end());
				float minY = *std::min_element(cornerYs.begin(), cornerYs.end());
				float maxY = *std::max_element(cornerYs.begin(), cornerYs.end());
				float minZ = *std::min_element(cornerZs.begin(), cornerZs.end());
				float maxZ = *std::max_element(cornerZs.begin(), cornerZs.end());

				currBoundingBox.minPosition = Math::Vector3{ minX, minY, minZ };
				currBoundingBox.maxPosition = Math::Vector3{ maxX, maxY, maxZ };

				return currBoundingBox;
			};

			// ������ռ��¼���˸�����İ�Χ��
			mCascadeBoundingBoxsInWorldSpace[i] = getFrustumBoundingBox(currFrustumCornersInWorldSpace);

			// ��ƽ��ͷ��İ˸�����ת������Դ�ռ��£��������ڹ�Դ�ռ��¼�����
			FrustumCorners currFrustumCornersInLightSpace;
			for (int32_t j = 0; j < 4; j++) {
				currFrustumCornersInLightSpace.nearCorners[j] = currFrustumCornersInWorldSpace.nearCorners[j].TransformAsPoint(sunViewMatrix);
				currFrustumCornersInLightSpace.farCorners[j] = currFrustumCornersInWorldSpace.farCorners[j].TransformAsPoint(sunViewMatrix);
			}

			// Զƽ��ĶԽ���
			float farPlaneDiagonalLength = (currFrustumCornersInLightSpace.farCorners[2] - currFrustumCornersInLightSpace.farCorners[0]).Length();
			// ƽ��ͷ��ĶԽ���
			float frustumDiagonalLength = (currFrustumCornersInLightSpace.farCorners[2] - currFrustumCornersInLightSpace.nearCorners[0]).Length();
			// �������ֵ
			float maxLength = farPlaneDiagonalLength > frustumDiagonalLength ? farPlaneDiagonalLength : frustumDiagonalLength;

			// ��Դ�ռ��µİ�Χ�е���������
			Math::BoundingBox boundingBoxFromSunLight = getFrustumBoundingBox(currFrustumCornersInLightSpace);
			float minX = boundingBoxFromSunLight.minPosition.x;
			float maxX = boundingBoxFromSunLight.maxPosition.x;
			float minY = boundingBoxFromSunLight.minPosition.y;
			float maxY = boundingBoxFromSunLight.maxPosition.y;
			float minZ = boundingBoxFromSunLight.minPosition.z;
			float maxZ = boundingBoxFromSunLight.maxPosition.z;

			float fWorldUnitsPerTexel = maxLength / (float)smCascadeShadowDepthTextureResolution;
			float posX = (minX + maxX) * 0.5f;
			posX /= fWorldUnitsPerTexel;
			posX = std::floor(posX);
			posX *= fWorldUnitsPerTexel;
			
			float posY = (minY + maxY) * 0.5f;
			posY /= fWorldUnitsPerTexel;
			posY = std::floor(posY);
			posY *= fWorldUnitsPerTexel;

			float posZ = minZ;
			posZ /= fWorldUnitsPerTexel;
			posZ = std::floor(posZ);
			posZ *= fWorldUnitsPerTexel;

			Math::Vector3 centerPos{ posX, posY, posZ };

			// �����Դ����ľ���
			mCascadeShadowLightCameraViewMatrixs[i] = sunViewMatrix;
			mCascadeShadowLightCameraProjectMatrixs[i] = DirectX::XMMatrixOrthographicLH(maxLength, maxLength, 0.0f, maxZ - minZ);
		}
	}

	void ShadowSystem::SpawnWorldSpaceFrustumCornersByIndex(int32_t index, const GPUCamera& gpuCamera, FrustumCorners& currFrustumCorners) {
		// �����ƽ����Զƽ��
		float nearPlaneRatio = 0.0f;
		float farPlaneRatio = 0.0f;

		for (int32_t i = 0; i < index; i++) {
			if (i > 0) {
				nearPlaneRatio += smShadowCascadesSplitRatio[i - 1];
			}
			farPlaneRatio += smShadowCascadesSplitRatio[i];
		}

		float nearPlane = (index == 0) ? gpuCamera.nearPlane : nearPlaneRatio * smCascadeShadowDistance;
		float farPlane = farPlaneRatio * smCascadeShadowDistance;

		// ��ȡ�������ת���VP����
		const Math::Matrix4 invVpMatrix = gpuCamera.viewProjection.Transpose().Inverse();

		// �Ӳü��ռ䷴ת������ռ�
		Math::Vector3 vecFrustum[8];
		vecFrustum[0] = Math::Vector3(-1.0f, -1.0f, 0.0f);		// xyz
		vecFrustum[1] = Math::Vector3(1.0f, -1.0f, 0.0f);		// Xyz
		vecFrustum[2] = Math::Vector3(-1.0f, 1.0f, 0.0f);		// xYz
		vecFrustum[3] = Math::Vector3(1.0f, 1.0f, 0.0f);		// XYz
		vecFrustum[4] = Math::Vector3(-1.0f, -1.0f, 1.0f);		// xyZ
		vecFrustum[5] = Math::Vector3(1.0f, -1.0f, 1.0f);		// XyZ
		vecFrustum[6] = Math::Vector3(-1.0f, 1.0f, 1.0f);		// xYZ
		vecFrustum[7] = Math::Vector3(1.0f, 1.0f, 1.0f);		// XYZ

		for (int i = 0; i < 8; i++) {
			vecFrustum[i] = vecFrustum[i].TransformAsPoint(invVpMatrix);
		}

		currFrustumCorners.nearCorners[0] = vecFrustum[0];
		currFrustumCorners.nearCorners[1] = vecFrustum[1];
		currFrustumCorners.nearCorners[2] = vecFrustum[2];
		currFrustumCorners.nearCorners[3] = vecFrustum[3];

		currFrustumCorners.farCorners[0] = vecFrustum[4];
		currFrustumCorners.farCorners[1] = vecFrustum[5];
		currFrustumCorners.farCorners[2] = vecFrustum[6];
		currFrustumCorners.farCorners[3] = vecFrustum[7];
	}

}