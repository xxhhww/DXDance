#pragma once
#include <thread>

namespace Renderer {

	/*
	* 资源上传器，目前设想的主要功能如下：
	* Uploader启动时应该拥有两个线程：命令提交线程 与 监视线程
	* 其中，命令提交线程负责执行UploadList中表示的任务，监视线程则负责对当前正在执行的UploadList进行监视，并在UploadList的状态发生转变时调用执行一些对应的操作(可以是回调函数)
	* Uploader其本身也负责UploadList的分配与回收工作
	*/
	class Uploader {
	public:

	private:

	};

}