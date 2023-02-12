#pragma once
#include <string>

namespace Tool {
	class OutputMemoryStream {
	public:
		OutputMemoryStream();
		OutputMemoryStream(const std::string& str);
		OutputMemoryStream(OutputMemoryStream&& rhs);
		OutputMemoryStream(const OutputMemoryStream& rhs);
		~OutputMemoryStream();
		void operator=(const OutputMemoryStream& rhs);
		void operator=(OutputMemoryStream&& rhs);

		template<typename T>
		bool Write(const T& value);
		bool Write(const void* data, uint64_t size);

		const void* Data() const;
		std::string Str() const;
		uint64_t Size() const;
		uint64_t Capacity() const;
	private:
		void Reserve(uint64_t size);
	private:
		char* mData{ nullptr };
		uint64_t mSize{ 0u };
		uint64_t mCapacity{ 0u };
	};

	class InputMemoryStream {
	public:
		explicit InputMemoryStream(const OutputMemoryStream& blob);

		template<typename T>
		void Read(T& v);
		bool Read(void* data, uint64_t size);

		const void* Data() const;
		uint64_t Size() const;
		uint64_t Pos() const;
	private:
		const char* mData{ nullptr };
		uint64_t mSize{ 0u };
		uint64_t mPos{ 0u };
	};
}

#include "MemoryStream.inl"