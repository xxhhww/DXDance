#include "MemoryStream.h"
#include <assert.h>

namespace Tool {
	OutputMemoryStream::OutputMemoryStream()
	: mData(nullptr)
	, mSize(0u)
	, mCapacity(0u) {}

	OutputMemoryStream::OutputMemoryStream(const std::string& str) {
		mData = new char[str.size()];
		memcpy(mData, str.data(), str.size());
		mSize = str.size();
		mCapacity = mSize;
	}

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

	void OutputMemoryStream::operator=(OutputMemoryStream&& rhs) {
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

	const void* OutputMemoryStream::Data() const {
		return mData;
	}

	std::string OutputMemoryStream::Str() const {
		return std::string(mData);
	}

	uint64_t OutputMemoryStream::Size() const {
		return mSize;
	}

	uint64_t OutputMemoryStream::Capacity() const {
		return mCapacity;
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

	const void* InputMemoryStream::Data() const {
		return mData;
	}

	uint64_t InputMemoryStream::Size() const {
		return mSize;
	}

	uint64_t InputMemoryStream::Pos() const {
		return mPos;
	}
}