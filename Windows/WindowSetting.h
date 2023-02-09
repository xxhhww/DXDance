#pragma once
#include <string>
#include "ECursor.h"

namespace Windows {
	struct WindowSetting {
		/*
		* Title of the window (Displayed in the title bar)
		*/
		std::string title;

		/*
		* Width in pixels of the window
		*/
		uint16_t width;

		/*
		* Height in pixels of the window
		*/
		uint16_t height;

		/*
		* ָ�������Ƿ�Ϊȫ��ģʽ(�ޱ߿򡢱���������������)���ߴ���ģʽ(�б߿��б�����)
		*/
		bool fullscreen = false;

		/*
		* ָ�������Ƿ�����ţ�ֻ�ڴ���ģʽʱ��Ч
		*/
		bool resizable = true;

		/*
		* ָ�����ڴ���ʱ�Ƿ���󻯣�ֻ�ڴ���ģʽʱ��Ч
		*/
		bool maximized = true;

		/*
		* ָ�����ڴ���ʱ�Ƿ�ɼ�
		*/
		bool visible = true;
	};
}