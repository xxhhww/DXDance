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
		uint16_t width = 1920;

		/*
		* Height in pixels of the window
		*/
		uint16_t height = 1080;

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
		bool maximized = false;

		/*
		* ָ�����ڳ�ʼ��ʱ�Ƿ�ɼ���ֻ�ڴ���ģʽʱ��Ч
		*/
		bool visible = true;
	};
}