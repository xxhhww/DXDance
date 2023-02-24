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
		* �����µĿؼ�
		*/
		template <typename T, typename ...Args>
		T& CreateWidget(Args&&... args);

		/*
		* �ӳٴ����µĿؼ�
		* �ú�������ҪĿ������ȷӦ���ڿؼ�����ʱ�����Ŀؼ���Ӳ���
		* �ڻ��ƿؼ�ʱ�����ֱ����������ؼ����ܻᵼ���ڴ����
		* ��ˣ�����ڻ��ƿؼ�ʱ��Ҫ��ӿؼ�����ʹ�ñ��������ú����ὫҪ��ӵĿؼ�����׼������������һ֡����֮ǰ��׼������ȡ����������mWidgets�н��л���
		*/
		template<typename T, typename ...Args>
		T& CreateWidgetDelay(Args&&... args);

		/*
		* ɾ���ؼ�
		*/
		void DeleteWidget(IWidget* widget);

		/*
		* ɾ�����пؼ�
		*/
		void DeleteAllWidgets();

		/*
		* �ӳ�ɾ�����пؼ�
		* �ú�������ҪĿ������ȷӦ���ڿؼ�����ʱ�����Ŀؼ�ɾ������
		* �ڻ��ƿؼ�ʱ�����ֱ��ɾ���ؼ����ܻᵼ���ڴ����
		* ��ˣ�����ڻ��ƿؼ�ʱ��Ҫɾ���ؼ�����ʹ�ñ��������ú����Ὣ���е�Widget���Ϊ����״̬��������һ֡����֮ǰ������ٹ���
		*/
		void DestoryAllWidgets();

		/*
		* �ؼ�����
		*/
		void DrawWidgets();

		/*
		* ��׼��������µĿؼ�
		*/
		void DoPreparation();

		/*
		* ɾ�������Ϊ���ٵ�Widget
		*/
		void DoDestruction();
	protected:
		std::vector<std::pair<IWidget*, IWidgetMangement>> mWidgets;	// ��Ҫÿ֡���ƵĿؼ�
		std::queue<IWidget*> mDelayWidgets;								// ��һ֡����ӵĿؼ�
	public:
		inline const auto& GetWidgets() const { return mWidgets; }
	};
}

#include "IWidgetContainer.inl"