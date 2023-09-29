#pragma once
#include "Windows/Window.h"
#include "Windows/InputManger.h"

#include "Tools/Clock.h"

#include "Renderer/RenderEngine.h"

#include "GHL/DebugLayer.h"

namespace Game {

	class Context {
	public:
		/*
		* ��ʼ����ע����ķ���
		*/
		Context(const std::string& name, HINSTANCE hInstance, int nCmdShow);

		/*
		* �Ƴ����������ķ���
		*/
		~Context();

	public:
		std::unique_ptr<Windows::Window>		 window;
		std::unique_ptr<Windows::InputManger>	 inputManger;
		std::unique_ptr<Tool::Clock>			 clock;			// ��ʱ��


		std::unique_ptr<Renderer::RenderEngine>  renderEngine;
	};

}