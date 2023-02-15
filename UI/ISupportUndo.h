#pragma once
#include <stack>
#include "Tools/MemoryStream.h"

namespace App {
	/* Undo接口类，是任何支持Undo操作的类的基类 */
	class ISupportUndo {
	protected:
		struct Undo {
			Tool::OutputMemoryStream blob;
		};

		/*
		* 向栈中压入一个Undo操作
		*/
		void PushUndo();

		/*
		* 从栈中弹出(撤销)一个Undo操作
		*/
		void PopUndo();

		/*
		* 注册Undo起始点
		*/
		void RegisterOrigin(Tool::OutputMemoryStream& blob);
		
		/*
		* 更新Undo起始点
		*/
		void UpdateOrigin();

		/*
		* 获取Undo起始点
		*/
		const auto& GetOriginBlob() const;

		virtual void Serialize(Tool::OutputMemoryStream& blob) = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;
	private:
		std::stack<Undo> mUndos;
		Tool::OutputMemoryStream mOriginalBlob;
	};
}