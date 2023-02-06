#pragma once

namespace UI {
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
		bool closable = false;
		bool resizable = true;
		bool movable = true;
		bool dockable = false;
		bool scrollable = true;
		bool hideBackground = false;
		bool forceHorizontalScrollbar = false;
		bool forceVerticalScrollbar = false;
		bool allowHorizontalScrollbar = false;
		bool bringToFrontOnFocus = true;
		bool collapsable = false;
		bool allowInputs = true;
		bool titleBar = true;
		bool autoSize = false;
	};
}