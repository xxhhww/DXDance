#pragma once
#include "Renderer/HierarchyInstancedStaticMesh.h"


namespace Renderer {

	class RenderEngine;

	class DetailObjectSystem {
	public:
		DetailObjectSystem(RenderEngine* renderEngine);
		~DetailObjectSystem() = default;

		/*
		* ��ʼ��
		*/
		void Initialize(RenderEngine* renderEngine);
		
		/*
		* ִ��CPU�޳�����
		*/
		void Update(const Math::Vector2& cameraPosition);
		
		/*
		* ���Pass
		*/
		void AddPass(RenderEngine* renderEngine);

	private:
		RenderEngine* mRenderEngine = nullptr;

		std::unique_ptr<HierarchyInstancedStaticMesh> mHierarchyInstancedStaticTreeMesh;
	};

}