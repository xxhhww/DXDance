#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Adapter.h"

namespace GHL {
	/*
	* 图形设备
	*/
	class Device : public D3DObject {
	public:
		/*
		* 构造函数
		*/
		Device(const Adapter& adapter, bool aftermathEnabled = false);

		/*
		* 启用AftermathEnabled
		*/
		void EnableAftermath(bool enable);

		/*
		* Get方法
		*/
		inline const auto  D3DDevice()             const { return mDevice.Get(); }
		inline const auto& SupportUniversalHeaps() const { return mSupportUniversalHeaps; }
		inline const auto& IsAftermathEnabled()    const { return mMinimumHeapSize; }
		inline const auto& GetMinimumHeapSize()    const { return mMinimumHeapSize; }
		inline const auto& GetHeapAlignment()      const { return mHeapAlignment; }
		inline const auto& GetNodeMask()           const { return mNodeMask; }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

	private:
		Microsoft::WRL::ComPtr<ID3D12Device8> mDevice;

		bool mSupportUniversalHeaps{ false };
		bool mAftermathEnabled{ false };
		uint64_t mMinimumHeapSize{ 0u };
		uint64_t mHeapAlignment{ 0u };
		uint64_t mNodeMask{ 0u };
	};
}