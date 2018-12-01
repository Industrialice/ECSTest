#pragma once

namespace StdLib
{
    class DIWRSpinLock
    {
        using atomicType = i32;

        static constexpr atomicType InclusiveBit = 31;
        static constexpr atomicType InclusiveMask = 1 << 31;
        static constexpr atomicType ExclusiveBit = 30;
        static constexpr atomicType ExclusiveMask = 1 << 30;
        static constexpr atomicType PendingExclusiveMask = 0x3FFF'0000;
        static constexpr atomicType ReadersMask = 0xFFFF;

        std::atomic<atomicType> _users{0};

    public:
        enum class LockType { Read, Exclusive, Inclusive };

        DIWRSpinLock() = default;
        DIWRSpinLock(DIWRSpinLock &&) = default;
        DIWRSpinLock &operator = (DIWRSpinLock &&) = default;

        void Lock(LockType type);
        void Unlock(LockType type);
        void InclusiveToExclusive();
    };
}