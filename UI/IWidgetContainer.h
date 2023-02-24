#pragma once
#include "IWidget.h"
#include <vector>
#include <queue>

namespace UI {
	enum class IWidgetMangement {
		InternalMangement = 0,
		ExtraMangement	  = 1
	};

	class IWidgetContainer {
	public:
		void RegisterWidget(IWidget* widget);
		void UnregisterWidget(IWidget* widget);

		/*
		* 创建新的控件
		*/
		template <typename T, typename ...Args>
		T& CreateWidget(Args&&... args);

		/*
		* 延迟创建新的控件
		* 该函数的主要目的是正确应对在控件绘制时发生的控件添加操作
		* 在绘制控件时，如果直接添加其他控件可能会导致内存出错
		* 因此，如果在绘制控件时需要添加控件，则使用本函数，该函数会将要添加的控件塞入准备区，并在下一帧绘制之前从准备区中取出，并塞入mWidgets中进行绘制
		*/
		template<typename T, typename ...Args>
		T& CreateWidgetDelay(Args&&... args);

		/*
		* 删除控件
		*/
		void DeleteWidget(IWidget* widget);

		/*
		* 删除所有控件
		*/
		void DeleteAllWidgets();

		/*
		* 延迟删除所有控件
		* 该函数的主要目的是正确应对在控件绘制时发生的控件删除操作
		* 在绘制控件时，如果直接删除控件可能会导致内存出错
		* 因此，如果在绘制控件时需要删除控件，则使用本函数，该函数会将所有的Widget标记为销毁状态，并在下一帧绘制之前完成销毁工作
		*/
		void DestoryAllWidgets();

		/*
		* 控件绘制
		*/
		void DrawWidgets();

		/*
		* 从准备区添加新的控件
		*/
		void DoPreparation();

		/*
		* 删除被标记为销毁的Widget
		*/
		void DoDestruction();
	protected:
		std::vector<std::pair<IWidget*, IWidgetMangement>> mWidgets;	// 需要每帧绘制的控件
		std::queue<IWidget*> mDelayWidgets;								// 下一帧待添加的控件
	public:
		inline const auto& GetWidgets() const { return mWidgets; }
	};
}

#include "IWidgetContainer.inl"