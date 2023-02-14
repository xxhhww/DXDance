#include "Editor.h"
#include "MenuBar.h"
#include "UI/PanelWindow.h"
#include "UI/MenuList.h"
#include "UI/Group.h"
#include "UI/TreeNode.h"
#include "UI/InputText.h"

namespace App {
	/*
	* �༭����ʼ��
	*/
	Editor::Editor(Context& context)
	: mContext(context) {
		/* ������Panels */

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
	* �༭������
	*/
	Editor::~Editor() {
		mCanvas.DeleteAllPanels();
	}

	/*
	* �༭�����º���
	*/
	void Editor::Update(float delta) {
		// ���±༭������
		// ����Actor����Ϊ�߼�
		// ��Ⱦ����
		// ���ƻ���
		DrawEditorPanels();
	}

	/*
	* ���Ʊ༭�����еĿ������
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}