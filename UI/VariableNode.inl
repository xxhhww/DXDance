#pragma once
#include "VariableNode.h"

namespace UI {
	template<typename TData>
	void VariableNode<TData>::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
		blob.Write(mValue);
		blob.Write(mMin);
		blob.Write(mMax);
		// mName 分为 长度与数据
		blob.Write(mName.size());
		blob.Write(mName.data(), mName.size());
		blob.Write(isExposed);
	}

	template<typename TData>
	void VariableNode<TData>::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		blob.Read(mValue);
		blob.Read(mMin);
		blob.Read(mMax);
		// 读取mName的 长度与数据
		size_t strSize{ 0u };
		blob.Read(strSize);
		blob.Read(mName.data(), strSize);
		blob.Read(isExposed);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}
}