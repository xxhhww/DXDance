#pragma once
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {
	class TreeNode : public IWidget, public IWidgetContainer {
	public:
		TreeNode(const std::string& name, bool isLeaf = false);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> openedEvent;
		Tool::Event<> closedEvent;
		Tool::Event<> clickedEvent;
		Tool::Event<> doubleClickedEvent;
	private:
		std::string mName;
		bool mIsLeaf;
		bool mOpenStatus{ false };
	};
}