#pragma once
#include <string>
#include <unordered_map>

namespace Core {

	class AssetPathDataBase {
	public:
		/*
		* 默认构造函数
		*/
		AssetPathDataBase() = default;

		/*
		* 构造函数
		*/
		AssetPathDataBase(const std::string& tablePath);

		/*
		* 析构函数
		*/
		~AssetPathDataBase();

		/*
		* 通过ID获得路径
		*/
		std::string GetPath(int64_t id);

		/*
		* 通过路径获取ID
		*/
		int64_t GetUID(const std::string& path);

		/*
		* 设置ID对应的路径
		*/
		void SetPath(int64_t id, const std::string& path);

		/*
		* 从磁盘中读取路径表
		*/
		void LoadFromDisk();

		/*
		* 写入磁盘
		*/
		void SaveToDisk();

	private:
		std::string mTablePath; // 路径表的绝对路径
		std::unordered_map<int64_t, std::string> mPathMap; // UID-路径对照表
	};

}