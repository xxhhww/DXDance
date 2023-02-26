#pragma once
#include <string>
#include <unordered_map>

namespace Tool {

	/*
	* MetaFile是存储资产元数据键值对的文件
	*/
	class MetaFile {
	public:
		using AttributePair = std::pair<std::string, std::string>;
		using AttributeMap = std::unordered_map<std::string, std::string>;

		/*
		* 从磁盘中加载键值对文件
		*/
		MetaFile(const std::string& filepath);


		/*
		* 保持当前内存中的键值对到Meta文件中
		*/
		void Save() const;

		/*
		* Return the value attached to the given key
		* If the key doesn't exist, a default value is returned (0, false, "NULL")
		*/
		template<typename T>
		T Get(const std::string& p_key);

		/*
		* Return the value attached to the given key
		* If the key doesn't exist, the specified value is returned
		*/
		template<typename T>
		T GetOrDefault(const std::string& p_key, T p_default);

		/*
		* Set a new value to the given key (Not applied to the real file untill Save() is called)
		*/
		template<typename T>
		bool Set(const std::string& p_key, const T& p_value);

		/*
		* Add a new key/value to the meta file object (Not applied to the real file untill Save() is called)
		*/
		template<typename T>
		bool Add(const std::string& p_key, const T& p_value);

		/*
		* Remove an key/value pair identified by the given key (Not applied to the real file untill Save() is called)
		*/
		bool Remove(const std::string& p_key);

		/*
		* Remove all key/value pairs (Not applied to the real file untill Save() is called)
		*/
		void RemoveAll();

		/*
		* Verify if the given key exists
		*/
		bool IsKeyExisting(const std::string& p_key) const;

		/*
		* Get the content stored in the meta file as a vector of strings (Each string correspond to an attribute pair : Attribute=Value
		*/
		std::vector<std::string> GetFormattedContent() const;

	private:
		void RegisterPair(const std::string& p_key, const std::string& p_value);
		void RegisterPair(const AttributePair& p_pair);

		void Load();

		AttributePair	ExtractKeyAndValue(const std::string& p_attributeLine)	const;
		bool			IsValidLine(const std::string& p_attributeLine)			const;
		bool			StringToBoolean(const std::string& p_value)				const;

	private:
		std::string		m_filePath;
		AttributeMap	m_data;
	};
}

#include "MetaFile.inl"