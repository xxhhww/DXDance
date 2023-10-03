#include "Game/AssetManger.h"
#include "Core/ServiceLocator.h"

namespace Game {

	AssetManger::AssetManger(Renderer::RenderEngine* renderEngine, const std::string& path)
	: mModelsPath(path + "/Models") {

		auto& graphicsKernel = *renderEngine;
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
	}

	AssetManger::~AssetManger() {
	}

	Renderer::Model* AssetManger::GetModel(const std::string& name) {
		auto it = mModels.find(name);
		return (it == mModels.end()) ? nullptr : it->second.get();
	}

	Renderer::Mesh* AssetManger::GetMesh(const std::string& name) {
		auto* model = GetModel(name);
		return model->GetFirstMesh();
	}

}