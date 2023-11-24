#include "Renderer/CascadeShadowPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/LinearBufferAllocator.h"

#include "Tools/Assert.h"

namespace Renderer {

	void CascadeShadowPass::Initialize(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		mCascadedFrustumPlanesInWorldSpace.resize(smNumShadowCascades);
		mCascadedLightCameraGpuVpMatrixs.resize(smNumShadowCascades);
		mCascadeShadowDepthTextures.resize(smNumShadowCascades);

		// 创建CascadeShadowDepthTexture
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

	void CascadeShadowPass::AddPass(RenderEngine* renderEngine) {
		auto* renderGraph = renderEngine->mRenderGraph.get();

		auto& finalOutputDesc =
			renderGraph->GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph->AddPass(
			"CascadeShadowCullingPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				// 根据级联阴影的层级数创建对应个数的
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;


			}
			);

		renderGraph->AddPass(
			"CascadeShadowRenderPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;


			}
		);
	}

	void CascadeShadowPass::Update(RenderEngine* renderEngine) {
		ASSERT_FORMAT(renderEngine->mPipelineResourceStorage->rootLightDataPerFrame.size() > 0, "SunLight Missing");
		UpdateCascadeShadowLightCameraMatrix(renderEngine->mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera, renderEngine->mPipelineResourceStorage->rootLightDataPerFrame[0]);
	}

	void CascadeShadowPass::UpdateCascadeShadowLightCameraMatrix(const GPUCamera& gpuCamera, const GPULight& sunLight) {
		// 太阳光方向及其ViewMatrix
		Math::Vector3 lightDirection{ sunLight.position.x, sunLight.position.y, sunLight.position.z };
		Math::Matrix4 lightViewMatrix = DirectX::XMMatrixLookToLH(Math::Vector3{ 0.0f, 0.0f, 0.0f }, lightDirection, Math::Vector3{ 0.0f, 1.0f, 0.0f });
		Math::Matrix4 invLightViewMatrix = lightViewMatrix.Inverse();

		for (int32_t i = 0; i < smNumShadowCascades; i++) {
			// 获取当前渲染相机下，当前级联层级的平截头体的八个顶点
			FrustumCorners currFrustumCornersInWorldSpace;
			GetWorldSpaceFrustumCornersByIndex(i, gpuCamera, currFrustumCornersInWorldSpace);

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

			// 将平截头体的八个顶点转换到光源空间下，后续均在光源空间下计算结果
			FrustumCorners currFrustumCornersInLightSpace;
			for (int32_t j = 0; j < 4; j++) {
				currFrustumCornersInLightSpace.nearCorners[j] = currFrustumCornersInWorldSpace.nearCorners[j].TransformAsPoint(lightViewMatrix);
				currFrustumCornersInLightSpace.farCorners[j] = currFrustumCornersInWorldSpace.farCorners[j].TransformAsPoint(lightViewMatrix);
			}

			// 远平面的对角线
			float farPlaneDiagonalLength = (currFrustumCornersInLightSpace.farCorners[2] - currFrustumCornersInLightSpace.farCorners[0]).Length();
			// 平截头体的对角线
			float frustumDiagonalLength = (currFrustumCornersInLightSpace.farCorners[2] - currFrustumCornersInLightSpace.nearCorners[0]).Length();
			// 计算最大值
			float maxLength = farPlaneDiagonalLength > frustumDiagonalLength ? farPlaneDiagonalLength : frustumDiagonalLength;

			// 光源空间下的包围盒的两个顶点
			Math::BoundingBox boundingBoxInLightSpace = getFrustumBoundingBox(currFrustumCornersInLightSpace);
			float minX = boundingBoxInLightSpace.minPosition.x;
			float maxX = boundingBoxInLightSpace.maxPosition.x;
			float minY = boundingBoxInLightSpace.minPosition.y;
			float maxY = boundingBoxInLightSpace.maxPosition.y;
			float minZ = boundingBoxInLightSpace.minPosition.z;
			float maxZ = boundingBoxInLightSpace.maxPosition.z;

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

			Math::Vector3 centerPosInLightSpace{ posX, posY, posZ };
			Math::Vector3 centerPosInWorldSpace = centerPosInLightSpace.TransformAsPoint(invLightViewMatrix);
			// 计算光源相机的矩阵与平截头体平面
			Math::Matrix4 viewMatrix = DirectX::XMMatrixLookToLH(centerPosInWorldSpace, lightDirection, Math::Vector3{ 0.0f, 1.0f, 0.0f });
			Math::Matrix4 projMatrix = DirectX::XMMatrixOrthographicLH(maxLength, maxLength, 0.0f, maxZ - minZ);
			mCascadedLightCameraGpuVpMatrixs[i] = (viewMatrix * projMatrix).Transpose();
			Math::Frustum::BuildFrustumPlanes(mCascadedLightCameraGpuVpMatrixs[i], mCascadedFrustumPlanesInWorldSpace[i].data());
		}
	}

	void CascadeShadowPass::GetWorldSpaceFrustumCornersByIndex(int32_t index, const GPUCamera& gpuCamera, FrustumCorners& currFrustumCorners) {
		// 计算近平面与远平面
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

		// 获取摄像机反转后的VP矩阵
		const Math::Matrix4 invVpMatrix = gpuCamera.viewProjection.Transpose().Inverse();

		// 从裁剪空间反转回世界空间
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