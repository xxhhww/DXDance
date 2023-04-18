#pragma once
#include <string>
#include <Windows.h>

namespace Windows {
	struct WindowSetting {
		/*
		* Title of the window (Displayed in the title bar)
		*/
		std::string title{ "DXDance" };

		/*
		* Width in pixels of the window
		*/
		uint16_t width = 2560u;

		/*
		* Height in pixels of the window
		*/
		uint16_t height = 1600u;

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
		* ָ�����ڳ�ʼ��ʱ�Ƿ�ɼ���ֻ�ڴ���ģʽʱ��Ч
		*/
		bool visible = true;
	};
}