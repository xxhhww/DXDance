#pragma once
#include "UI/IWidget.h"

#include "Tools/Event.h"

namespace Core {
	class Actor;
}

namespace App {

	class Hierarchy;

	class HierarchyItem : public UI::IWidget {
		friend class Hierarchy;
	public:
		HierarchyItem(Core::Actor* actor, bool root = false);
		~HierarchyItem();

		template<typename T, typename ...Args>
		inline T* CreateHierarchyItem(Args&&... args) {
			static_assert(std::is_base_of<IWidget, T>::value, "T should derive from IWidget");
			T* item = new T(args...);
			item->AttachParent(this);
			ConsiderItem(item);
			return item;
		}

		void AttachParent(HierarchyItem* parent);

		void ConsiderItem(HierarchyItem* targetItem);

		HierarchyItem* UnConsiderItem(HierarchyItem* targetItem);

	protected:
		void _Draw_Internal_Impl() override;

		/*
		* 删除被标记为销毁的Widget
		*/
		void DoDestruction();

	public:
		Tool::Event<> openedEvent;
		Tool::Event<> closedEvent;
		Tool::Event<> clickedEvent;
		Tool::Event<> doubleClickedEvent;
		bool opened{ false };

	private:
		bool root{ false };
		Core::Actor* mActor{ nullptr };
		HierarchyItem* mParent{ nullptr };
		std::vector<HierarchyItem*> mChilds;
	};

}