#pragma once

namespace UI {
	enum class UIStyle {
		CLASSIC,
		DARK,
		LIGHT,
	};

	enum class HorizontalAlignment {
		LEFT,
		CENTER,
		RIGHT
	};

	enum class VerticalAlignment {
		TOP,
		MIDDLE,
		BOTTOM
	};

	struct PanelWindowSettings {
		bool closable = true;
		bool resizable = true;
		bool movable = true;
		bool dockable = true;
		bool scrollable = true;
		bool hideBackground = false;
		bool forceHorizontalScrollbar = false;
		bool forceVerticalScrollbar = false;
		bool allowHorizontalScrollbar = false;
		bool bringToFrontOnFocus = true;
		bool collapsable = true;
		bool allowInputs = true;
		bool titleBar = true;
		bool menuBar = true;
		bool autoSize = true;
	};
}