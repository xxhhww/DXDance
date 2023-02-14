#pragma once
#include "Context.h"

namespace App {
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
		* 编辑器更新函数
		*/
		void Update(float delta);

		/*
		* 绘制编辑器所有的控制面板
		*/
		void DrawEditorPanels();
	private:
		Context& mContext;	// 引擎上下文
		UI::Canvas mCanvas;	// 编辑器画布
	};
}