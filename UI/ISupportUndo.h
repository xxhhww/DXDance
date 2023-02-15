#pragma once
#include <stack>
#include "Tools/MemoryStream.h"

namespace App {
	/* Undo�ӿ��࣬���κ�֧��Undo��������Ļ��� */
	class ISupportUndo {
	protected:
		struct Undo {
			Tool::OutputMemoryStream blob;
		};

		/*
		* ��ջ��ѹ��һ��Undo����
		*/
		void PushUndo();

		/*
		* ��ջ�е���(����)һ��Undo����
		*/
		void PopUndo();

		/*
		* ע��Undo��ʼ��
		*/
		void RegisterOrigin(Tool::OutputMemoryStream& blob);
		
		/*
		* ����Undo��ʼ��
		*/
		void UpdateOrigin();

		/*
		* ��ȡUndo��ʼ��
		*/
		const auto& GetOriginBlob() const;

		virtual void Serialize(Tool::OutputMemoryStream& blob) = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;
	private:
		std::stack<Undo> mUndos;
		Tool::OutputMemoryStream mOriginalBlob;
	};
}