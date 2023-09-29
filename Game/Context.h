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
		* 初始化并注册核心服务
		*/
		Context(const std::string& name, HINSTANCE hInstance, int nCmdShow);

		/*
		* 移除并析构核心服务
		*/
		~Context();

	public:
		std::unique_ptr<Windows::Window>		 window;
		std::unique_ptr<Windows::InputManger>	 inputManger;
		std::unique_ptr<Tool::Clock>			 clock;			// 定时器


		std::unique_ptr<Renderer::RenderEngine>  renderEngine;
	};

}