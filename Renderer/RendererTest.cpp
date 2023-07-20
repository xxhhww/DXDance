#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/CommandBuffer.h"

#include "Renderer/TerrainOfflineTask.h"
#include "Renderer/GenerateCloudNoiseTask.h"

#include "Windows/Window.h"
#include "Windows/InputManger.h"

#include "GHL/DebugLayer.h"

#include "ECS/CTransform.h"
#include "ECS/CCamera.h"
#include "ECS/CLight.h"
#include "ECS/CSky.h"
#include "ECS/Entity.h"

#include "Tools/Clock.h"

using namespace Renderer;
using namespace Windows;

static uint32_t sWindowWidth = 979u;
static uint32_t sWindowHeight = 635u;

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

void RunRenderer() {
    GHL::EnableDebugLayer();

    WindowSetting setting{};
    setting.width = sWindowWidth;
    setting.height = sWindowHeight;
    setting.fullscreen = false;
    setting.maximized = false;
    Window window{ setting };
    InputManger inputManger{ &window };
    Tool::Clock clock;

    RenderEngine renderEngine(window.GetHWND(), setting.width, setting.height);
    // Test_RenderGraphBuildDAG(renderEngine);

    // EditorCamera
    ECS::Camera editorCamera;
    editorCamera.frustum.farZ = 5000.0f;
    editorCamera.translationSpeed *= 10.0f;
    ECS::Transform editorTransform;
    editorTransform.worldPosition.x = 0.0f;
    editorTransform.worldPosition.y = 900.0f;
    editorTransform.worldPosition.z = 0.0f;

    // MainCamera(RenderCamera)
    {
        auto mainCamera = ECS::Entity::Create<ECS::Transform, ECS::Camera>();
        
        auto& transform = mainCamera.GetComponent<ECS::Transform>();
        transform.worldPosition = Math::Vector3{ 0.0f, 900.0f, 0.0f };

        auto& camera = mainCamera.GetComponent<ECS::Camera>();
        camera.cameraType = ECS::CameraType::RenderCamera;
        camera.mainCamera = true;
    }

    // Sky
    {
        auto skyEntity = ECS::Entity::Create<ECS::Transform, ECS::Sky>();
        
        auto& transform = skyEntity.GetComponent<ECS::Transform>();
        transform.worldRotation = Math::Vector3{ DirectX::XM_PIDIV4, 0.0f, 0.0f };

        auto& sky = skyEntity.GetComponent<ECS::Sky>();
    }

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

        // 计算VP矩阵
        camera.viewProjMatrix = camera.viewMatrix * camera.projMatrix;
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

        renderEngine.Update(clock.GetDeltaTime(), clock.GetTimeSinceStart(), editorCamera, editorTransform);
        renderEngine.Render();

        inputManger.PostUpdate();

        clock.Update();
    }
}

void DoTerrainOfflineTask() {
    GHL::EnableDebugLayer();

    WindowSetting setting{};
    setting.width = sWindowWidth;
    setting.height = sWindowHeight;
    setting.fullscreen = false;
    setting.maximized = false;
    Window window{ setting };

    RenderEngine renderEngine(window.GetHWND(), setting.width, setting.height);
    TerrainOfflineTask _TerrainOfflineTask;
    _TerrainOfflineTask.Initialize(
        "E:/MyProject/DXDance/Resources/Textures/HeightMap.png",
        &renderEngine
    );
    renderEngine.mOfflineTaskPass += std::bind(
        &TerrainOfflineTask::Generate, &_TerrainOfflineTask,
        std::placeholders::_1, std::placeholders::_2);

    renderEngine.mOfflineCompletedCallback += std::bind(
        &TerrainOfflineTask::OnCompleted, &_TerrainOfflineTask
    );

    Tool::Clock clock;
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

        renderEngine.DoOfflineTask();
        clock.Update();
    }

}

void DoGenerateCloudNoiseTask() {
    GHL::EnableDebugLayer();

    WindowSetting setting{};
    setting.width = sWindowWidth;
    setting.height = sWindowHeight;
    setting.fullscreen = false;
    setting.maximized = false;
    Window window{ setting };

    RenderEngine renderEngine(window.GetHWND(), setting.width, setting.height);

    GenerateCloudNoiseTask _GenerateCloudNoiseTask;

    _GenerateCloudNoiseTask.Initialize(&renderEngine);
    renderEngine.mOfflineTaskPass += std::bind(
        &GenerateCloudNoiseTask::Generate, &_GenerateCloudNoiseTask,
        std::placeholders::_1, std::placeholders::_2);

    renderEngine.mOfflineCompletedCallback += std::bind(
        &GenerateCloudNoiseTask::OnCompleted, &_GenerateCloudNoiseTask
    );

    Tool::Clock clock;
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

        renderEngine.DoOfflineTask();
        clock.Update();
    }
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    RunRenderer();
    // DoTerrainOfflineTask();
    // DoGenerateCloudNoiseTask();
}