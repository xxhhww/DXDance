#include "EditorAssetManger.h"

#include "Core/ServiceLocator.h"

#include "Renderer/RenderEngine.h"

namespace Core {

	EditorAssetManger::EditorAssetManger(const std::string& path)
		: mModelsPath(path + "/Models") {

		auto& graphicsKernel = CORESERVICE(Renderer::RenderEngine);
		auto* device = graphicsKernel.mDevice.get();
		auto* descriptorAllocator = graphicsKernel.mDescriptorAllocator.get();
		auto* heapAllocator = graphicsKernel.mHeapAllocator.get();
		auto* uploader = graphicsKernel.mUploaderEngine.get();

		mModels["Cube"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Cube.fbx");
		mModels["Cube"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Cylinder"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Cylinder.fbx");
		mModels["Cylinder"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Plane"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Plane.fbx");
		mModels["Plane"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Vertical_Plane"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Vertical_Plane.fbx");
		mModels["Vertical_Plane"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Roll"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Roll.fbx");
		mModels["Roll"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Sphere"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Sphere.fbx");
		mModels["Sphere"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Arrow_Translate"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Arrow_Translate.fbx");
		mModels["Arrow_Translate"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Arrow_Rotate"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Arrow_Rotate.fbx");
		mModels["Arrow_Rotate"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Arrow_Scale"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Arrow_Scale.fbx");
		mModels["Arrow_Scale"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Arrow_Picking"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Arrow_Picking.fbx");
		mModels["Arrow_Picking"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());

		mModels["Camera"] = std::make_unique<Renderer::Model>(device, descriptorAllocator, heapAllocator, mModelsPath + "/Camera.fbx");
		mModels["Camera"]->LoadDataFromDisk(uploader->GetMemoryCopyQueue(), uploader->GetCopyFence());
	
		auto* shaderManger = graphicsKernel.mShaderManger.get();
		
		shaderManger->CreateGraphicsShader("AxisPicking",
			[](Renderer::GraphicsStateProxy& proxy) {
				proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Editor/AxisPicking.hlsl";
				proxy.psFilepath = proxy.vsFilepath;
			});
		mGraphicsShaders["AxisPicking"] = shaderManger->GetShader<Renderer::GraphicsShader>("AxisPicking");

		shaderManger->CreateGraphicsShader("AxisRender",
			[](Renderer::GraphicsStateProxy& proxy) {
				proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Editor/AxisRender.hlsl";
				proxy.psFilepath = proxy.vsFilepath;
			});
		mGraphicsShaders["AxisRender"] = shaderManger->GetShader<Renderer::GraphicsShader>("AxisRender");
	}

	EditorAssetManger::~EditorAssetManger() {

	}

	Renderer::Model* EditorAssetManger::GetModel(const std::string& name) {
		auto it = mModels.find(name);
		return (it == mModels.end()) ? nullptr : it->second.get();
	}

	Renderer::Mesh* EditorAssetManger::GetMesh(const std::string& name) {
		auto* model = GetModel(name);
		return model->GetFirstMesh();
	}

	Renderer::GraphicsShader* EditorAssetManger::GetShader(const std::string& name) {
		auto it = mGraphicsShaders.find(name);
		return (it == mGraphicsShaders.end()) ? nullptr : it->second;
	}

}