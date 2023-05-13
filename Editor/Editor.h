#pragma once
#include "Context.h"

namespace App {
	class ShaderEditor;
	class AssetBrowser;
	class Hierarchy;
	class SceneView;

	class Editor {
	public:
		/*
		* 编辑器初始化
		*/
		Editor(Context& context);

		/*
		* 编辑器析构
		*/
		~Editor();

		/*
		* 编辑器运行函数
		*/
		void Run();

		/*
		* 编辑器更新函数
		*/
		void Update(float delta);

		/*
		* 绘制编辑器所有的控制面板
		*/
		void DrawEditorPanels();
	private:
		/*
		* 编辑器预处理函数
		*/
		void PreUpdate(float delta);

		/*
		* 编辑器后处理函数
		*/
		void PostUpdate(float delta);

		/*
		* 更新编辑器面板
		*/
		void UpdateEditorPanels(float delta);

		/*
		* 更新视图面板
		*/
		void UpdateAndRenderViewPanels(float delta);

	private:
		Context& mContext;	// 引擎上下文
		UI::Canvas mCanvas;	// 编辑器画布
		ShaderEditor* mShaderEditor{ nullptr };
		AssetBrowser* mAssetBrowser{ nullptr };
		Hierarchy*    mHierarchy{ nullptr };
		SceneView*    mSceneView{ nullptr };
	};
}