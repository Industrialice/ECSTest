#include "PreHeader.hpp"
#include "DIWRSpinLock.hpp"

using namespace StdLib;

// TODO: optimize memory_order

void DIWRSpinLock::Lock(LockType type)
{
    switch (type)
    {
    case DIWRSpinLock::LockType::Read: // exclusive and pending exclusive must be 0, inclusive and read don't matter
        for (auto oldLock = _users.load();;)
        {
            oldLock &= ReadersMask | InclusiveMask;
            auto newLock = oldLock + 1;
            if (_users.compare_exchange_weak(oldLock, newLock))
            {
                break;
            }
        }
        break;
    case DIWRSpinLock::LockType::Exclusive: // inclusive, exclusive and read must be 0, pending exclusive doesn't matter
        _users.fetch_add(ReadersMask + 1);
        for (auto oldLock = _users.load();;)
        {
            oldLock &= PendingExclusiveMask;
            atomicType newLock = ExclusiveMask | oldLock;
            if (_users.compare_exchange_weak(oldLock, newLock))
            {
                break;
            }
        }
        break;
    case DIWRSpinLock::LockType::Inclusive: // inclusive, exclusive and pending exclusive must be 0, read doesn't matter
        for (auto oldLock = _users.load();;)
        {
            oldLock &= ReadersMask;
            auto newLock = oldLock | InclusiveMask;
            if (_users.compare_exchange_weak(oldLock, newLock))
            {
                break;
            }
        }
        break;
    }
}

void DIWRSpinLock::Unlock(LockType type)
{
    switch (type)
    {
    case DIWRSpinLock::LockType::Read:
        ASSUME((_users.load() & ReadersMask) > 0);
        ASSUME(!Funcs::IsBitSet(_users.load(), ExclusiveBit));
        _users.fetch_sub(1);
        break;
    case DIWRSpinLock::LockType::Exclusive:
        ASSUME(Funcs::IsBitSet(_users.load(), ExclusiveBit));
        ASSUME(!Funcs::IsBitSet(_users.load(), InclusiveBit));
        ASSUME((_users.load() & ReadersMask) == 0);
        ASSUME((_users.load() & PendingExclusiveMask) != 0);
        _users.fetch_sub((ReadersMask + 1) | ExclusiveMask);
        break;
    case DIWRSpinLock::LockType::Inclusive:
        ASSUME(!Funcs::IsBitSet(_users.load(), ExclusiveBit));
        ASSUME(Funcs::IsBitSet(_users.load(), InclusiveBit));
        _users.fetch_sub(InclusiveMask);
        break;
    }
}

void DIWRSpinLock::InclusiveToExclusive()
{
    ASSUME(Funcs::IsBitSet(_users.load(), InclusiveBit));
    ASSUME(!Funcs::IsBitSet(_users.load(), ExclusiveBit));
    _users.fetch_add(ReadersMask + 1);
    for (auto oldLock = _users.load();;)
    {
        oldLock &= PendingExclusiveMask;
        auto newLock = oldLock | ExclusiveMask;
        oldLock |= InclusiveMask;
        if (_users.compare_exchange_weak(oldLock, newLock))
        {
            break;
        }
    }
}
