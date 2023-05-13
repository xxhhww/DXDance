#pragma once
#include "IDrawable.h"
#include "IWidgetContainer.h"
#include "Pluginable.h"

namespace UI {
	class IPanel : public IDrawable, public Pluginable, public IWidgetContainer {
	public:
		IPanel();
		virtual ~IPanel() = default;

		void Draw() override;
	protected:
		virtual void _Draw_Internal_Impl() = 0;
	protected:
		bool mEnable{ true };
		std::string mPlaneID{ "?" };
		static int64_t smPlaneIDInc;

		bool mFirstFrame{ true };
	public:
		inline const auto& GetPlaneID() const { return mPlaneID; }
		inline auto IsEnable() const { return mEnable; }
	};
}