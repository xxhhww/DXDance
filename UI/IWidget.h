#pragma once
#include "IDrawable.h"
#include <string>

namespace UI {
	class IWidgetContainer;

	class IWidget : public IDrawable {
	public:
		IWidget();
		virtual ~IWidget() = default;

		void Destory();
		void SetParent(IWidgetContainer* parent);
		void Draw() override;
	protected:
		virtual void _Draw_Internal_Impl() = 0;
	protected:
		bool mEnable{ true };
		bool mDestory{ false };
		bool mLineBreak{ true };
		IWidgetContainer* mParent{ nullptr };
		std::string mWidgetID{ "?" };
	private:
		static int64_t smWidgetIDInc;
	public:
		inline auto IsDestory() const { return mDestory; }
		inline auto IsEnabled() const { return mEnable; }
		inline auto GetParent() const { return mParent; }
		inline const auto& GetID() const { return mWidgetID; }
	};
}