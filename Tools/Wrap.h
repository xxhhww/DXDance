#pragma once
#include <functional>

namespace Tool {

	/*
	* °ü×°
	*/
	template<typename T>
	class Wrap {
	public:
		Wrap() = default;
		Wrap(T* wrapResource, const std::function<void()>& deletedCallBack)
		: mWrapResource(wrapResource)
		, mDeletedCallBack(deletedCallBack) {}
		Wrap(const Wrap& other) = delete;
		Wrap(Wrap&& other) = default;
		Wrap& operator=(const Wrap& other) = delete;
		Wrap& operator=(Wrap&& other) = default;

		~Wrap() {
			if (mDeletedCallBack != nullptr) {
				mDeletedCallBack();
			}
		}

		inline operator T* ()		{ return mWrapResource; }
		inline operator T* () const { return mWrapResource; }

		inline T* operator->() { return mWrapResource; }
		
		inline T* Get() const { return mWrapResource; }

		inline void Release() {
			if (mDeletedCallBack != nullptr) {
				mDeletedCallBack();
				mDeletedCallBack = nullptr;
			}
		}

	private:
		T* mWrapResource{ nullptr };
		std::function<void()> mDeletedCallBack{ nullptr };
	};

}