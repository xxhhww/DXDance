#pragma once
#include "Renderer/HierarchyInstancedStaticMesh.h"


namespace Renderer {

	class RenderEngine;

	class DetailObjectSystem {
	public:
		DetailObjectSystem(RenderEngine* renderEngine);
		~DetailObjectSystem() = default;

		/*
		* 初始化
		*/
		void Initialize(RenderEngine* renderEngine);
		
		/*
		* 执行CPU剔除操作
		*/
		void Update(const Math::Vector2& cameraPosition);
		
		/*
		* 添加Pass
		*/
		void AddPass(RenderEngine* renderEngine);

	private:
		RenderEngine* mRenderEngine = nullptr;

		std::unique_ptr<HierarchyInstancedStaticMesh> mHierarchyInstancedStaticTreeMesh;
	};

}