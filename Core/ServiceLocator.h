#pragma once
#include <any>
#include <unordered_map>

#define CORESERVICE(T) Core::ServiceLocator::Get<T>()

namespace Core {
	/**
	* Provide a way to access to core services
	*/
	class ServiceLocator {
	public:
		/**
		* Register a service in the service locator
		* @param p_service
		*/
		template<typename T>
		static void Provide(T& p_service)
		{
			smServices[typeid(T).hash_code()] = reinterpret_cast<void*>(&p_service);
		}

		/**
		* Returns a service of the given type (Make sure that your provided the service before calling this method)
		*/
		template<typename T>
		static T& Get()
		{
			return *reinterpret_cast<T*>(smServices[typeid(T).hash_code()]);
		}

		static void RemoveAllServices() {
			smServices.clear();
		}

	private:
		inline static std::unordered_map<size_t, void*> smServices;
	};
}