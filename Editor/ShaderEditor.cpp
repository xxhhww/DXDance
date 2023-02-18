#include "ShaderEditor.h"
#include "Core/ServiceLocator.h"
#include "Windows/InputManger.h"
#include <sstream>

namespace App {
	/*
	* 构造函数
	*/
	ShaderEditor::ShaderEditor(
		const std::string& name,
		bool opened,
		const UI::PanelWindowSettings& panelSettings) 
	: ISupportUndoWindow(name, opened, panelSettings) {
		auto& menuBar = CreateWidget<UI::MenuBar>();
		CreateFuncsMenu(menuBar);
		CreateMiniMapMenu(menuBar);
		CreateNodesMenu(menuBar);

		// 创建Graph
		mGraph = std::make_unique<Graph>();

		// 设置原初状态
		Tool::OutputMemoryStream blob;
		Serialize(blob);
		RegisterOrigin(blob);
	}

	/*
	* 处理快捷键输入
	*/
	void ShaderEditor::HandleShortCut() {
	}

	/*
	* 创建方法菜单
	*/
	void ShaderEditor::CreateFuncsMenu(UI::MenuBar& menuBar) {
		auto& menu = menuBar.CreateWidget<UI::MenuList>("Function");
		menu.CreateWidget<UI::MenuItem>("Close").clickedEvent += std::bind(&UI::PanelWindow::Close, this);
		menu.CreateWidget<UI::MenuItem>("Compile").clickedEvent += std::bind(&ShaderEditor::Compile, this);
	}

	/*
	* 创建小窗口菜单
	*/
	void ShaderEditor::CreateMiniMapMenu(UI::MenuBar& menuBar) {
		auto& menu = menuBar.CreateWidget<UI::MenuList>("MiniMap");

		mMiniMapMenuItem[0] = &menu.CreateWidget<UI::MenuItem>("TopLeft", true, false);
		mMiniMapMenuItem[1] = &menu.CreateWidget<UI::MenuItem>("TopRight", true, false);
		mMiniMapMenuItem[2] = &menu.CreateWidget<UI::MenuItem>("BottomLeft", true, false);
		mMiniMapMenuItem[3] = &menu.CreateWidget<UI::MenuItem>("BottomRight", true, true);
		mMinimapLocation = ImNodesMiniMapLocation_BottomRight;

		mMiniMapMenuItem[0]->clickedEvent += [&]() {
			mMiniMapMenuItem[0]->checkStatus = true;
			mMiniMapMenuItem[1]->checkStatus = false;
			mMiniMapMenuItem[2]->checkStatus = false;
			mMiniMapMenuItem[3]->checkStatus = false;
			mMinimapLocation = ImNodesMiniMapLocation_TopLeft;
		};

		mMiniMapMenuItem[1]->clickedEvent += [&]() {
			mMiniMapMenuItem[0]->checkStatus = false;
			mMiniMapMenuItem[1]->checkStatus = true;
			mMiniMapMenuItem[2]->checkStatus = false;
			mMiniMapMenuItem[3]->checkStatus = false;
			mMinimapLocation = ImNodesMiniMapLocation_TopRight;
		};

		mMiniMapMenuItem[2]->clickedEvent += [&]() {
			mMiniMapMenuItem[0]->checkStatus = false;
			mMiniMapMenuItem[1]->checkStatus = false;
			mMiniMapMenuItem[2]->checkStatus = true;
			mMiniMapMenuItem[3]->checkStatus = false;
			mMinimapLocation = ImNodesMiniMapLocation_BottomLeft;
		};

		mMiniMapMenuItem[3]->clickedEvent += [&]() {
			mMiniMapMenuItem[0]->checkStatus = false;
			mMiniMapMenuItem[1]->checkStatus = false;
			mMiniMapMenuItem[2]->checkStatus = false;
			mMiniMapMenuItem[3]->checkStatus = true;
			mMinimapLocation = ImNodesMiniMapLocation_BottomRight;
		};
	}

	/*
	* 创建节点菜单
	*/
	void ShaderEditor::CreateNodesMenu(UI::MenuBar& menuBar) {
		auto& menu = menuBar.CreateWidget<UI::MenuList>("Node");

		auto lambda = [this](const NodeType& nodeType) {
			// 创建节点并设置节点坐标
			std::unique_ptr<Node> node = Node::CreateNode(mNodeIncID++, nodeType);
			ImVec2 clickPos = ImGui::GetMousePos();
			ImNodes::SetNodeScreenSpacePos(node->objectID, clickPos);
			ImVec2 nodePosition = ImNodes::GetNodeEditorSpacePos(node->objectID);
			node->SetPosition(nodePosition.x, nodePosition.y);
			this->mGraph->PushNode(node);
			this->PushUndo();
		};
		auto& mathNodeMenu = menu.CreateWidget<UI::MenuList>("Math");
		mathNodeMenu.CreateWidget<UI::MenuItem>("Add").clickedEvent += std::bind(lambda, NodeType::Add);
		mathNodeMenu.CreateWidget<UI::MenuItem>("Subtract");
		mathNodeMenu.CreateWidget<UI::MenuItem>("Multiply");
		mathNodeMenu.CreateWidget<UI::MenuItem>("Divide");
		mathNodeMenu.CreateWidget<UI::MenuItem>("Cos").clickedEvent += std::bind(lambda, NodeType::Cos);;

		auto& variableNodeMenu = menu.CreateWidget<UI::MenuList>("Variable");
		variableNodeMenu.CreateWidget<UI::MenuItem>("Bool");
		variableNodeMenu.CreateWidget<UI::MenuItem>("Float").clickedEvent	+= std::bind(lambda, NodeType::Float);
		variableNodeMenu.CreateWidget<UI::MenuItem>("Float2").clickedEvent	+= std::bind(lambda, NodeType::Float2);
		variableNodeMenu.CreateWidget<UI::MenuItem>("Float3").clickedEvent	+= std::bind(lambda, NodeType::Float3);
		variableNodeMenu.CreateWidget<UI::MenuItem>("Float4").clickedEvent	+= std::bind(lambda, NodeType::Float4);
		variableNodeMenu.CreateWidget<UI::MenuItem>("Color").clickedEvent	+= std::bind(lambda, NodeType::Color);

		auto& rootNodeMenu = menu.CreateWidget<UI::MenuList>("Root");
		rootNodeMenu.CreateWidget<UI::MenuItem>("BRDF");
	}

	void ShaderEditor::_Draw_Internal_Impl() {
		if (mOpened) {
			int windowFlags = ImGuiWindowFlags_None;

			if (!resizable)					windowFlags |= ImGuiWindowFlags_NoResize;
			if (!movable)					windowFlags |= ImGuiWindowFlags_NoMove;
			if (!dockable)					windowFlags |= ImGuiWindowFlags_NoDocking;
			if (hideBackground)				windowFlags |= ImGuiWindowFlags_NoBackground;
			if (forceHorizontalScrollbar)	windowFlags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
			if (forceVerticalScrollbar)		windowFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			if (allowHorizontalScrollbar)	windowFlags |= ImGuiWindowFlags_HorizontalScrollbar;
			if (!bringToFrontOnFocus)		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
			if (!collapsable)				windowFlags |= ImGuiWindowFlags_NoCollapse;
			if (!allowInputs)				windowFlags |= ImGuiWindowFlags_NoInputs;
			if (!scrollable)                windowFlags |= ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
			if (!titleBar)					windowFlags |= ImGuiWindowFlags_NoTitleBar;
			if (menuBar)					windowFlags |= ImGuiWindowFlags_MenuBar;

			ImVec2 minSizeConstraint{ minSize.x, minSize.y };
			ImVec2 maxSizeConstraint{ maxSize.x, maxSize.y };

			/* Cancel constraint if x or y is <= 0.f */
			if (minSizeConstraint.x <= 0.0f || minSizeConstraint.y <= 0.0f)
				minSizeConstraint = { 0.0f, 0.0f };

			if (maxSizeConstraint.x <= 0.0f || maxSizeConstraint.y <= 0.0f)
				maxSizeConstraint = { 10000.0f, 10000.0f };

			ImGui::SetNextWindowSizeConstraints(minSizeConstraint, maxSizeConstraint);

			if (ImGui::Begin((name + mPlaneID).c_str(), closable ? &mOpened : nullptr, windowFlags)) {
				mHovered = ImGui::IsWindowHovered();
				mFocused = ImGui::IsWindowFocused();

				auto scrollY = ImGui::GetScrollY();

				mScrolledToBottom = scrollY == ImGui::GetScrollMaxY();
				mScrolledToTop = scrollY == 0.0f;

				// 除了初始化时默认开启外，控制界面为Open状态有且仅有通过点击菜单栏的View
				// 控制界面为Close状态除了菜单栏的Window外，还可以通过点击Panel自带的x图标来实现关闭
				// 综上所述，此处需要对Panel的状态进行判定
				if (!mOpened)
					closedEvent.Invoke();

				Update();

				if (mMustScrollToBottom) {
					ImGui::SetScrollY(ImGui::GetScrollMaxY());
					mMustScrollToBottom = false;
				}

				if (mMustScrollToTop) {
					ImGui::SetScrollY(0.0f);
					mMustScrollToTop = false;
				}

				// 绘制MenuBar
				DrawWidgets();

				// 绘制Editor
				ImNodes::BeginNodeEditor();

				for (auto& pair : mGraph->GetNodeMap()) {
					if (pair.second->Draw()) {
						ISupportUndoWindow::PushUndo();
					}
				}
				for (auto& pair : mGraph->GetLinkMap()) {
					pair.second->Draw();
				}

				ImNodes::MiniMap(0.2f, mMinimapLocation);
				ImNodes::EndNodeEditor();

				// Link Created(包含隐式的Link Deleted)
				{
					int startPin, endPin;
					if (ImNodes::IsLinkCreated(&startPin, &endPin)) {
						std::unique_ptr<Link> link = std::make_unique<Link>(mLinkIncID++, startPin, endPin);
						mGraph->PushLink(link);
						ISupportUndoWindow::PushUndo();
					}
				}

				// Link & Node Deleted
				{
					if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_X)) {
						bool isDeleted{ false };
						// Link Deleted
						{
							const int selectedLinkCount = ImNodes::NumSelectedLinks();
							if (selectedLinkCount > 0) {
								std::vector<int> linkSelected;
								linkSelected.resize(static_cast<size_t>(selectedLinkCount));
								ImNodes::GetSelectedLinks(linkSelected.data());
								for (const auto& id : linkSelected) {
									mGraph->EraseLink(id);
								}
								isDeleted = true;
							}
						}
						// Node Deleted
						{
							const int selectedNodeCount = ImNodes::NumSelectedNodes();
							if (selectedNodeCount > 0) {
								std::vector<int> nodeSelected;
								nodeSelected.resize(static_cast<size_t>(selectedNodeCount));
								ImNodes::GetSelectedNodes(nodeSelected.data());
								for (const auto& id : nodeSelected) {
									mGraph->EraseNode(id);
								}
								isDeleted = true;
							}
						}
						if (isDeleted) {
							ISupportUndoWindow::PushUndo();
						}
					}
				}

				// Node Moved
				{
					const int selectedNodeCount = ImNodes::NumSelectedNodes();
					if (selectedNodeCount > 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
						std::vector<int> nodeMoved;
						nodeMoved.resize(static_cast<size_t>(selectedNodeCount));
						ImNodes::GetSelectedNodes(nodeMoved.data());
						bool isMoved{ false };
						for (const auto& id : nodeMoved) {
							if (id == (int)0x80000000) continue;
							const auto newPos = ImNodes::GetNodeEditorSpacePos(id);
							auto& node = mGraph->GetNodeMap()[id];
							auto oldPos = node->GetPosition();
							if (oldPos.x != newPos.x || oldPos.y != newPos.y) isMoved = true;
							else isMoved = false;
							if (isMoved) {
								node->SetPosition(newPos.x, newPos.y);
							}
						}
						if (isMoved) {
							ISupportUndoWindow::PushUndo();
						}
					}
				}

				// Ctrl-Z
				{
					if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_LCTRL) &&
						CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_Z)) {
						ISupportUndoWindow::PopUndo();
					}
				}

				// Ctrl-S
				{
					if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_LCTRL) &&
						CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_S)) {
						ISupportUndoWindow::UpdateOrigin();
					}
				}
			}
			ImGui::End();
		}
	}

	void ShaderEditor::Serialize(Tool::OutputMemoryStream& blob)  const {
		blob.Write(mNodeIncID);
		blob.Write(mLinkIncID);

		const auto& nodes = mGraph->GetNodeMap();
		blob.Write(nodes.size());
		for (const auto& pair : nodes) {
			pair.second->Serialize(blob);
		}

		const auto& links = mGraph->GetLinkMap();
		blob.Write(links.size());
		for (const auto& pair : links) {
			pair.second->Serialize(blob);
		}
	}

	void ShaderEditor::Deserialize(Tool::InputMemoryStream& blob) {
		mGraph->Clear();

		blob.Read(mNodeIncID);
		blob.Read(mLinkIncID);

		size_t nodeCount{ 0u };
		blob.Read(nodeCount);
		for (uint32_t i = 0; i < nodeCount; i++) {
			NodeType type;
			int id;
			blob.Read(type);
			blob.Read(id);
			
			std::unique_ptr<Node> node = Node::CreateNode(id, type);
			node->Deserialize(blob);
			mGraph->PushNode(node);
		}

		size_t linkCount{ 0u };
		blob.Read(linkCount);
		for (uint32_t i = 0; i < linkCount; i++) {
			int objectID;
			int startPin;
			int endPin;
			blob.Read(objectID);
			blob.Read(startPin);
			blob.Read(endPin);
			
			std::unique_ptr<Link> link = std::make_unique<Link>(objectID, startPin, endPin);
			link->Deserialize(blob);
			mGraph->PushLink(link);
		}
	}
}