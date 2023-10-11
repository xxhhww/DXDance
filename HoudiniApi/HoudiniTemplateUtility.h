#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "Tools/Assert.h"
#include <functional>

namespace Houdini {

	inline static constexpr int HAPI_MAX_PAGE_SIZE = 20000;

	struct HoudiniTemplateUtility {
	public:
		template<typename T>
		using GetArray1ArgFunc = std::function<void(const HAPI_Session* session, int arg1, T* data, int start, int length)>;

		template<typename ARG2, typename T>
		using GetArray2ArgFunc = std::function<void(const HAPI_Session* session, int arg1, ARG2 arg2, T* data, int start, int length)>;

		template<typename ARG3, typename ARG2, typename T>
		using GetArray3ArgFunc = std::function<void(const HAPI_Session* session, int arg1, ARG2 arg2, ARG3 arg3, T* data, int start, int length)>;

	public:
		template<typename T>
		static void GetArray1Arg(const HAPI_Session* session, int arg1, GetArray1ArgFunc<T> func, T* data, int start, int count) {
			GetArray<int32_t, int32_t, T>(session, arg1, 0, 0, func, nullptr, nullptr, data, start, count, 1);
		}

		template<typename ARG2, typename T>
		static void GetArray2Arg(const HAPI_Session* session, int arg1, ARG2 arg2, GetArray2ArgFunc<ARG2, T> func, T* data, int start, int count) {
			GetArray<int32_t, ARG2, T>(session, arg1, arg2, 0, nullptr, func, nullptr, data, start, count, 1);
		}

		template<typename ARG3, typename ARG2, typename T>
		static void GetArray3Arg(const HAPI_Session* session, int arg1, ARG2 arg2, ARG3 arg3, GetArray3ArgFunc<ARG3, ARG2, T> func, T* data, int start, int count) {
			GetArray<ARG3, ARG2, T>(session, arg1, arg2, arg3, nullptr, nullptr, func, data, start, count, 1);
		}

		template<typename ARG3, typename ARG2, typename T>
		static void GetArray(
			const HAPI_Session* session,
			int arg1, ARG2 arg2, ARG3 arg3,
			GetArray1ArgFunc<T> func1,
			GetArray2ArgFunc<ARG2, T> func2,
			GetArray3ArgFunc<ARG3, ARG2, T> func3,
			T* data, int start, int count, int tupleSize) {

			int maxArraySize = HAPI_MAX_PAGE_SIZE / (sizeof(T) * tupleSize);
			int localCount = count;
			int currentIndex = start;

			while (localCount > 0) {
				int length = 0;
				if (localCount > maxArraySize) {
					length = maxArraySize;
					localCount -= maxArraySize;
				}
				else {
					length = localCount;
					localCount = 0;
				}

				std::vector<T> localArray(length * tupleSize);

				if (func1 != nullptr) {
					func1(session, arg1, localArray.data(), currentIndex, length);
				}
				else if (func2 != nullptr) {
					func2(session, arg1, arg2, localArray.data(), currentIndex, length);
				}
				else if (func3 != nullptr) {
					func3(session, arg1, arg2, arg3, localArray.data(), currentIndex, length);
				}
				else {
					ASSERT_FORMAT(false, "Func Empty!");
				}

				// Copy from temporary array
				for (int i = currentIndex; i < (currentIndex + length); ++i) {
					for (int j = 0; j < tupleSize; ++j) {
						data[i * tupleSize + j] = localArray[(i - currentIndex) * tupleSize + j];
					}
				}

				currentIndex += length;
			}
		}
	};

}