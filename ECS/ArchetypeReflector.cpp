#include "PreHeader.hpp"
#include "ArchetypeReflector.hpp"

using namespace ECSTest;

bool ArchetypeReflector::Contains(Archetype archetype) const
{
	_lock.Lock(DIWRSpinLock::LockType::Read);
	bool contains = _library.find(archetype) != _library.end();
	_lock.Unlock(DIWRSpinLock::LockType::Read);
	return contains;
}

void ArchetypeReflector::AddToLibrary(Archetype archetype, vector<StableTypeId> &&types)
{
	_lock.Lock(DIWRSpinLock::LockType::Exclusive);
	_library[archetype] = move(types);
	_lock.Unlock(DIWRSpinLock::LockType::Exclusive);
}

Array<const StableTypeId> ArchetypeReflector::Reflect(Archetype archetype) const
{
	_lock.Lock(DIWRSpinLock::LockType::Read);
	auto it = _library.find(archetype);
	ASSUME(it != _library.end());
	auto types = ToArray(it->second);
	_lock.Unlock(DIWRSpinLock::LockType::Read);
	return types;
}
