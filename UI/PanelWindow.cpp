#include "PanelWindow.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace UI {
	PanelWindow::PanelWindow
	(
		const std::string& name,
		bool opened,
		const PanelWindowSettings& panelSettings
	)
	: name(name)
	, resizable(panelSettings.resizable)
	, closable(panelSettings.closable)
	, movable(panelSettings.movable)
	, scrollable(panelSettings.scrollable)
	, dockable(panelSettings.dockable)
	, hideBackground(panelSettings.hideBackground)
	, forceHorizontalScrollbar(panelSettings.forceHorizontalScrollbar)
	, forceVerticalScrollbar(panelSettings.forceVerticalScrollbar)
	, allowHorizontalScrollbar(panelSettings.allowHorizontalScrollbar)
	, bringToFrontOnFocus(panelSettings.bringToFrontOnFocus)
	, collapsable(panelSettings.collapsable)
	, allowInputs(panelSettings.allowInputs)
	, mOpened(opened) {
		autoSize = panelSettings.autoSize;
	}

	void PanelWindow::SetOpen(bool status) {
		if (mOpened && !status) {
			closedEvent.Invoke();
		}
		else if (!mOpened && status) {
			openedEvent.Invoke();
		}
		mOpened = status;
	}

	void PanelWindow::Open() {
		if (!mOpened) {
			mOpened = true;
			openedEvent.Invoke();
		}
	}

	void PanelWindow::Close() {
		if (mOpened) {
			mOpened = false;
			closedEvent.Invoke();
		}
	}

	void PanelWindow::Focus() {
		ImGui::SetWindowFocus((name + mPlaneID).c_str());
	}

	bool PanelWindow::IsOpened() const {
		return mOpened;
	}

	bool PanelWindow::IsHovered() const {
		return mHovered;
	}

	bool PanelWindow::IsFocused() const {
		return mFocused;
	}

	bool PanelWindow::IsAppearing() const {
		const auto window = ImGui::FindWindowByName((name + mPlaneID).c_str());
		if (window)
			return window->Appearing;
		else
			return false;
	}

	void PanelWindow::ScrollToBottom() {
		mMustScrollToBottom = true;
	}

	void PanelWindow::ScrollToTop() {
		mMustScrollToTop = true;
	}

	bool PanelWindow::IsScrolledToBottom() const {
		return mMustScrollToBottom;
	}

	bool PanelWindow::IsScrolledToTop() const {
		return mMustScrollToTop;
	}

	void PanelWindow::_Draw_Internal_Impl() {
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

				// ���˳�ʼ��ʱĬ�Ͽ����⣬���ƽ���ΪOpen״̬���ҽ���ͨ������˵�����View
				// ���ƽ���ΪClose״̬���˲˵�����View�⣬������ͨ�����Panel�Դ���xͼ����ʵ�ֹر�
				// �����������˴���Ҫ��Panel��״̬�����ж�
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
				DrawWidgets();
			}
			ImGui::End();
		}
	}
}