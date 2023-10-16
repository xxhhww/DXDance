#pragma once
#include "Windows/Window.h"
#include "Windows/InputManger.h"

#include "Tools/Clock.h"

#include "Renderer/RenderEngine.h"

#include "GHL/DebugLayer.h"

#include "Jolt/Jolt.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/Core/JobSystemThreadPool.h"

#include "Physics/PhysicsSystem.h"

#include "Game/SystemManger.h"
#include "Game/GlobalSetting.h"
#include "Game/AssetManger.h"

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
		uint32_t maxJobs{ 2048u };
		uint32_t maxBarriers{ 8u };
		uint32_t numThreads;

		std::unique_ptr<Windows::Window>		 window;
		std::unique_ptr<Windows::InputManger>	 inputManger;
		std::unique_ptr<Tool::Clock>			 clock;			// 定时器

		std::unique_ptr<JPH::JobSystem>          jobSystem;

		std::unique_ptr<Physics::PhysicsSystem>  physicsSystem;

		std::unique_ptr<Renderer::RenderEngine>  renderEngine;

		std::unique_ptr<SystemManger>            systemManger;

		// 游戏的资产数据
		std::unique_ptr<AssetManger>             assetManger;

		// 游戏的全局数据
		std::unique_ptr<GlobalSetting>           globalSetting;
	};

}