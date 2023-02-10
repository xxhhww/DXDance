#include "MemoryStream.h"
#include <assert.h>

namespace Tool {
	OutputMemoryStream::OutputMemoryStream()
	: mData(nullptr)
	, mSize(0u)
	, mCapacity(0u) {}

	OutputMemoryStream::OutputMemoryStream(OutputMemoryStream&& rhs) {
		mSize = rhs.mSize;
		mCapacity = rhs.mCapacity;
		mData = rhs.mData;

		rhs.mCapacity = 0;
		rhs.mSize = 0;
		rhs.mData = nullptr;
	}

	OutputMemoryStream::OutputMemoryStream(const OutputMemoryStream& rhs) {
		if (rhs.mData != nullptr && rhs.mCapacity > 0u) {
			mData = new char[rhs.mCapacity];
			memcpy(mData, rhs.mData, rhs.mCapacity);
		}
		else {
			mData = nullptr;
		}
		mSize = rhs.mSize;
		mCapacity = rhs.mCapacity;
	}

	OutputMemoryStream::~OutputMemoryStream() {
		if (mData != nullptr) delete mData;
	}

	void OutputMemoryStream::operator=(const OutputMemoryStream& rhs) {
		if (mData != nullptr) delete mData;

		if (rhs.mData != nullptr && rhs.mCapacity > 0u) {
			mData = new char[rhs.mCapacity];
			memcpy(mData, rhs.mData, rhs.mCapacity);
		}
		else {
			mData = nullptr;
		}
		mSize = rhs.mSize;
		mCapacity = rhs.mCapacity;
	}

	void OutputMemoryStream::operator=(OutputMemoryStream&& rhs) noexcept {
		if (mData != nullptr) delete mData;

		mSize = rhs.mSize;
		mCapacity = rhs.mCapacity;
		mData = rhs.mData;

		rhs.mCapacity = 0;
		rhs.mSize = 0;
		rhs.mData = nullptr;
	}

	bool OutputMemoryStream::Write(const void* data, uint64_t size) {
		if (data == nullptr || size == 0u) {
			return true;
		}
		// À©ÈÝ
		if (mSize + size > mCapacity) {
			Reserve((mSize + size) << 1);
		}
		memcpy(mData + mSize, data, size);
		mSize += size;
		return true;
	}

	void OutputMemoryStream::Reserve(uint64_t size) {
		if (size <= mCapacity) return;

		char* tmp = new char[size];
		memcpy(tmp, mData, mCapacity);
		delete mData;
		mData = tmp;
		mCapacity = size;
	}

	InputMemoryStream::InputMemoryStream(const OutputMemoryStream& blob)
	: mData((const char*)blob.Data())
	, mSize(blob.Size())
	, mPos(0u) {}

	bool InputMemoryStream::Read(void* data, uint64_t size) {
		if (mPos + size > mSize) {
			assert(false);
			return false;
		}
		if (size > 0) memcpy(data, mData + mPos, size);
		mPos += size;
		return true;
	}
}