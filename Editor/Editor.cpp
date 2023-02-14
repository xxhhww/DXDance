#include "Editor.h"
#include "MenuBar.h"
#include "UI/PanelWindow.h"
#include "UI/MenuList.h"
#include "UI/Group.h"
#include "UI/TreeNode.h"
#include "UI/InputText.h"

namespace App {
	/*
	* 编辑器初始化
	*/
	Editor::Editor(Context& context)
	: mContext(context) {
		/* 测试用Panels */

		auto& menuBar = mCanvas.CreatePanel<MenuBar>();

		auto& panelWindow = mCanvas.CreatePanel<UI::PanelWindow>("TestPanel");
		auto& group1 = panelWindow.CreateWidget<UI::GroupCollapsable>("Group");
		group1.CreateWidget<UI::TreeNode>("Leaf1", true);
		std::string sss = "This is content";
		auto& inputText1 = group1.CreateWidget<UI::InputText>("InputText", sss);

		menuBar.RegisterPanel(&panelWindow);

		mContext.uiManger->SetCanvas(&mCanvas);
	}

	/*
	* 编辑器析构
	*/
	Editor::~Editor() {
		mCanvas.DeleteAllPanels();
	}

	/*
	* 编辑器更新函数
	*/
	void Editor::Update(float delta) {
		// 更新编辑器数据
		// 处理Actor的行为逻辑
		// 渲染数据
		// 绘制画布
		DrawEditorPanels();
	}

	/*
	* 绘制编辑器所有的控制面板
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}