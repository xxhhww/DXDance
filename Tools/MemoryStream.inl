#pragma once
#include "MemoryStream.h"

namespace Tool {
	template<typename T>
	bool OutputMemoryStream::Write(const T& value) {
		return this->Write(&value, sizeof(T));
	}
	
	template<typename T>
	void InputMemoryStream::Read(T& value) {
		this->Read(&value, sizeof(T));
	}
}