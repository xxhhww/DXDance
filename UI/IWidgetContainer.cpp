#include "IWidgetContainer.h"
#include <algorithm>

namespace UI {
	void IWidgetContainer::RegisterWidget(IWidget* widget) {
		mWidgets.emplace_back(widget, IWidgetMangement::ExtraMangement);
		widget->SetParent(this);
	}

	void IWidgetContainer::UnregisterWidget(IWidget* widget) {
		auto it = std::find_if(mWidgets.begin(), mWidgets.end(),
			[&widget](std::pair<IWidget*, IWidgetMangement>& pair) {
				return pair.first == widget;
			}
		);

		if (it != mWidgets.end()) {
			widget->SetParent(nullptr);
			mWidgets.erase(it);
		}
	}

	void IWidgetContainer::DeleteWidget(IWidget* widget) {
		auto it = std::find_if(mWidgets.begin(), mWidgets.end(),
			[&widget](std::pair<IWidget*, IWidgetMangement>& pair) {
				if (pair.first == widget && pair.second == IWidgetMangement::InternalMangement) {
					return true;
				}
				return false;
			}
		);

		if (it != mWidgets.end()) {
			mWidgets.erase(it);
			delete widget;
		}
	}

	void IWidgetContainer::DeleteAllWidgets() {
		std::for_each(mWidgets.begin(), mWidgets.end(),
			[](auto& pair) {
				if (pair.second == IWidgetMangement::InternalMangement) {
					delete pair.first;
				}
			});
		mWidgets.clear();
	}

	void IWidgetContainer::DestoryAllWidgets() {
		std::for_each(mWidgets.begin(), mWidgets.end(),
			[](auto& pair) {
				pair.first->Destory();
			});
	}

	void IWidgetContainer::DrawWidgets() {
		DoDestruction();
		DoPreparation();

		for (const auto& pair : mWidgets) {
			pair.first->Draw();
		}
	}

	void IWidgetContainer::DoPreparation() {
		while (!mDelayWidgets.empty()) {
			IWidget* widget = mDelayWidgets.front();
			mWidgets.emplace_back(widget, IWidgetMangement::InternalMangement);
			mDelayWidgets.pop();
		}
	}

	void IWidgetContainer::DoDestruction() {
		mWidgets.erase(
			std::remove_if(mWidgets.begin(), mWidgets.end(),
				[](std::pair<IWidget*, IWidgetMangement>& pair) {
					bool needErase = pair.first->IsDestory();
					if (needErase && pair.second == IWidgetMangement::InternalMangement) {
						delete pair.first;
					}
					return needErase;
				}), 
			mWidgets.end());
	}
}