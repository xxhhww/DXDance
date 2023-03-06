#pragma once
#include <string>
#include <unordered_map>

namespace Core {

	class AssetPathDataBase {
	public:
		/*
		* Ĭ�Ϲ��캯��
		*/
		AssetPathDataBase() = default;

		/*
		* ���캯��
		*/
		AssetPathDataBase(const std::string& tablePath);

		/*
		* ��������
		*/
		~AssetPathDataBase();

		/*
		* ͨ��ID���·��
		*/
		std::string GetPath(int64_t id);

		/*
		* ·���ı�
		*/
		void PathChanged(int64_t id, const std::string& newPath);

		/*
		* �Ӵ����ж�ȡ·����
		*/
		void LoadFromDisk();

		/*
		* д�����
		*/
		void SaveToDisk();

	private:
		std::string mTablePath; // ·����ľ���·��
		std::unordered_map<int64_t, std::string> mPathMap; // UID-·�����ձ�
	};

}