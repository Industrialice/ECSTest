#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class System
    {
    public:
        enum MessageType
        {
            ComponentChanged,
            ComponentRemoved,
            ComponentAdded,
            EntityRemoved,
            EntityAdded
        };

        enum ComponentAvailability
        {
            Required,
            Optional,
			Subtractive
        };

        struct RequestedComponent
        {
            StableTypeId type;
            bool isWriteAccess;
			ComponentAvailability isRequired;
        };

		[[nodiscard]] virtual pair<const RequestedComponent *, uiw> RequestedComponents() const = 0;
		[[nodiscard]] virtual StableTypeId Type() const = 0;
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem();
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const;
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem();
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const;
        virtual void Accept(void **array) = 0;
	};

	struct IndirectSystem : public System
	{
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem() override final;
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const override final;
        virtual void ProcessMessages(MessageType type, const void *messageHeader) = 0;
        virtual void Update() = 0;
	};

	struct DirectSystem : public System
	{
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem() override final;
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const override final;
	};

    template <typename BaseSystem, typename Type> struct _SystemTypeIdentefiable : public BaseSystem, public Type
    {
    public:
        using Type::GetTypeId;

        [[nodiscard]] virtual StableTypeId Type() const override final
        {
            return GetTypeId();
        }
    };

    #ifdef DEBUG
        #define INDIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentefiable<IndirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>>
    #else
        #define INDIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentefiable<IndirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>>
    #endif

    #ifdef DEBUG
        #define DIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentefiable<DirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>>
    #else
        #define DIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentefiable<DirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>>
    #endif
}

#include "SystemHelpers.hpp"