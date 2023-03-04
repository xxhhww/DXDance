#pragma once
#include "Actor.h"

namespace Core {
	template<typename Comp>
	Comp& Actor::AddComponent() {
		return mEntity.AddComponent<Comp>();
	}

	template<typename Comp>
	Comp& Actor::AddComponent(Comp& comp) {
		return mEntity.AddComponent(comp);
	}

	template<typename Comp>
	Comp& Actor::GetComponent() {
		return mEntity.GetComponent<Comp>();
	}

	template<typename Comp>
	const Comp& Actor::GetComponent() const {
		return mEntity.GetComponent<Comp>();
	}


	template<typename Comp>
	void Actor::DelComponent() {
		mEntity.DelComponent<Comp>();
	}

	template<typename Comp>
	bool Actor::HasComponent() const {
		return mEntity.HasComponent<Comp>();
	}
}