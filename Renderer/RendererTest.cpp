#include "RenderEngine.h"
#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "CommandBuffer.h"

#include "Windows/Window.h"
#include "Windows/InputManger.h"

#include "GHL/DebugLayer.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"
#include "ECS/Entity.h"

#include "Tools/Clock.h"

using namespace Renderer;
using namespace Windows;

void Test_RenderGraphBuildDAG(RenderEngine& renderEngine) {
    RenderGraph& renderGraph = *renderEngine.mRenderGraph.get();

    renderGraph.AddPass(
        "Pass0",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

            NewTextureProperties properties{};
            properties.width = 1920u;
            properties.height = 1080u;
            properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            // builder.NewTexture("Tex_0", properties);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
    );

    renderGraph.AddPass(
        "Pass1",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::PixelShader);

            NewTextureProperties properties{};
            properties.width = 1920u;
            properties.height = 1080u;
            properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            // builder.NewTexture("Tex_1", properties);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass2",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

            NewTextureProperties properties{};
            properties.width = 1920u;
            properties.height = 1080u;
            properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            builder.ReadTexture("Tex_0", ShaderAccessFlag::PixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass3",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);
            // builder.NewTexture("Tex_3", NewTextureProperties{});

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass4",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);
            builder.ReadTexture("Tex_1", ShaderAccessFlag::NonPixelShader);
            builder.ReadTexture("Tex_3", ShaderAccessFlag::NonPixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass5",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass6",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_1", ShaderAccessFlag::NonPixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.Build();
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    GHL::EnableDebugLayer();

    WindowSetting setting{};
    setting.width = 979u;
    setting.height = 635u;
    setting.fullscreen = false;
    setting.maximized = false;
    Window window{ setting };
    InputManger inputManger{ &window };
    Tool::Clock clock;

    RenderEngine renderEngine(window.GetHWND(), setting.width, setting.height);
    // Test_RenderGraphBuildDAG(renderEngine);

    // MainCamera(RenderCamera)
    auto mainCamera = ECS::Entity::Create<ECS::Transform, ECS::Camera>();
    auto& cTransform = mainCamera.GetComponent<ECS::Transform>();
    cTransform.worldPosition = Math::Vector3{ 0.0f, 10.0f, -3.0f };
    cTransform.worldRotation.y = 45.0f;
    auto& cCamera = mainCamera.AddComponent<ECS::Camera>();
    cCamera.cameraType = ECS::CameraType::RenderCamera;
    cCamera.mainCamera = true;

    // EditorCamera
    ECS::Camera editorCamera;

    editorCamera.lookUp = Math::Vector3{ 0.0339f, -0.2368f, 0.9709f };
    editorCamera.right = Math::Vector3{ 0.9993f, 0.0f, -0.0348f };
    editorCamera.up = Math::Vector3{ 0.00826f, 0.9715f, 0.2367f };

    ECS::Transform editorTransform;
    editorTransform.worldPosition.x = 4.4f;
    editorTransform.worldPosition.y = 3.6f;
    editorTransform.worldPosition.z = -21.7f;

    // 更新编辑摄像机与渲染摄像机的矩阵数据
    auto updateCameraMatrix = [](ECS::Camera& camera, ECS::Transform& transform) {
        // 计算ViewMatrix
        const XMVECTOR camTarget = transform.worldPosition + camera.lookUp;
        camera.viewMatrix = XMMatrixLookAtLH(
            transform.worldPosition,
            camTarget,
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        // 计算ProjMatrix
        camera.projMatrix = XMMatrixPerspectiveFovLH(
            camera.frustum.fovY,
            camera.frustum.aspect,
            camera.frustum.nearZ,
            camera.frustum.farZ);
    };

    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                done = true;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        if (done) {
            break;
        }

        float dt = clock.GetDeltaTime();
        inputManger.PreUpdate(dt);
        if (inputManger.IsMouseButtonPressed(Windows::EMouseButton::MOUSE_RBUTTON)) {
            // 处理摄像机移动
            if (inputManger.IsKeyDown(Windows::EKey::KEY_W)) {
                XMVECTOR s = XMVectorReplicate(dt * editorCamera.translationSpeed);
                XMVECTOR l = XMLoadFloat3(&editorCamera.lookUp);
                XMVECTOR p = XMLoadFloat3(&editorTransform.worldPosition);
                XMStoreFloat3(&editorTransform.worldPosition, XMVectorMultiplyAdd(s, l, p));
            }
            if (inputManger.IsKeyDown(Windows::EKey::KEY_S)) {
                XMVECTOR s = XMVectorReplicate(-dt * editorCamera.translationSpeed);
                XMVECTOR l = XMLoadFloat3(&editorCamera.lookUp);
                XMVECTOR p = XMLoadFloat3(&editorTransform.worldPosition);
                XMStoreFloat3(&editorTransform.worldPosition, XMVectorMultiplyAdd(s, l, p));
            }
            if (inputManger.IsKeyDown(Windows::EKey::KEY_A)) {
                XMVECTOR s = XMVectorReplicate(-dt * editorCamera.translationSpeed);
                XMVECTOR r = XMLoadFloat3(&editorCamera.right);
                XMVECTOR p = XMLoadFloat3(&editorTransform.worldPosition);
                XMStoreFloat3(&editorTransform.worldPosition, XMVectorMultiplyAdd(s, r, p));
            }
            if (inputManger.IsKeyDown(Windows::EKey::KEY_D)) {
                XMVECTOR s = XMVectorReplicate(dt * editorCamera.translationSpeed);
                XMVECTOR r = XMLoadFloat3(&editorCamera.right);
                XMVECTOR p = XMLoadFloat3(&editorTransform.worldPosition);
                XMStoreFloat3(&editorTransform.worldPosition, XMVectorMultiplyAdd(s, r, p));
            }
            // 处理摄像机旋转
            if (inputManger.IsMouseMove()) {
                Math::Vector2 rawDelta = inputManger.GetMouseRawDelta();

                XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&editorCamera.right), editorCamera.rotationSpeed * XMConvertToRadians((float)rawDelta.y));
                XMStoreFloat3(&editorCamera.up, XMVector3TransformNormal(XMLoadFloat3(&editorCamera.up), R));
                XMStoreFloat3(&editorCamera.lookUp, XMVector3TransformNormal(XMLoadFloat3(&editorCamera.lookUp), R));

                XMMATRIX R1 = XMMatrixRotationY(editorCamera.rotationSpeed * XMConvertToRadians((float)rawDelta));

                XMStoreFloat3(&editorCamera.right, XMVector3TransformNormal(XMLoadFloat3(&editorCamera.right), R1));
                XMStoreFloat3(&editorCamera.up, XMVector3TransformNormal(XMLoadFloat3(&editorCamera.up), R1));
                XMStoreFloat3(&editorCamera.lookUp, XMVector3TransformNormal(XMLoadFloat3(&editorCamera.lookUp), R1));
            }
        }

        updateCameraMatrix(editorCamera, editorTransform);
        ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
            const float rt = fmod(transform.worldRotation.y, DirectX::XM_2PI);

            if (rt > DirectX::XM_PI)
                transform.worldRotation.y = rt - DirectX::XM_2PI;
            else if (rt < -DirectX::XM_PI)
                transform.worldRotation.y = rt + DirectX::XM_2PI;

            camera.lookUp = XMVector4Transform(Math::Vector4{ 0.0f, 0.0f, 1.0f, 0.0f }, XMMatrixRotationRollPitchYaw(transform.worldRotation.x, transform.worldRotation.y, transform.worldRotation.z));
            camera.right = XMVector3Cross(Math::Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, camera.lookUp);
            camera.up = XMVector3Cross(camera.lookUp, camera.right);

            updateCameraMatrix(camera, transform);
        });
        
        renderEngine.Update(0.0f, editorCamera, editorTransform);
        renderEngine.Render();

        inputManger.PostUpdate();

        clock.Update();
    }

    return 0;
}