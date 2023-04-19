#pragma once
#include "IPanelTransformable.h"
#include "Tools/Event.h"

namespace UI {
	class PanelWindow : public IPanelTransformable {
	public:
		PanelWindow
		(
			const std::string& name = "",
			bool opened = true,
			const PanelWindowSettings& panelSettings = PanelWindowSettings{}
		);
		virtual ~PanelWindow() = default;

		void SetOpen(bool status);
		void Open();
		void Close();
		void Focus();
		bool IsOpened() const;
		bool IsHovered() const;
		bool IsFocused() const;
		bool IsAppearing() const;
		void ScrollToBottom();
		void ScrollToTop();
		bool IsScrolledToBottom() const;
		bool IsScrolledToTop() const;
	protected:
		virtual void _Draw_Internal_Impl() override;
	public:
		std::string name;
		Math::Vector2 minSize { 0.0f, 0.0f };
		Math::Vector2 maxSize { 0.0f, 0.0f };
		Tool::Event<> openedEvent{};
		Tool::Event<> closedEvent{};

		bool resizable = true;
		bool closable = false;
		bool movable = true;
		bool scrollable = true;
		bool dockable = false;
		bool hideBackground = false;
		bool forceHorizontalScrollbar = false;
		bool forceVerticalScrollbar = false;
		bool allowHorizontalScrollbar = false;
		bool bringToFrontOnFocus = true;
		bool collapsable = false;
		bool allowInputs = true;
		bool titleBar = true;
		bool menuBar = true;

	protected:
		bool mOpened;
		bool mHovered{ false };
		bool mFocused{ false };
		bool mMustScrollToBottom{ false };
		bool mMustScrollToTop{ false };
		bool mScrolledToBottom{ false };
		bool mScrolledToTop{ false };
	};
}