#pragma once

#include "Component.hpp"
#include "MessageBuilder.hpp"

namespace ECSTest
{
    class System
    {
    public:
        struct Environment
        {
            f32 timeSinceLastFrame;
            ui32 frameNumber;
            f64 timeSinceStarted;
            EntityIDGenerator &idGenerator;
        };

        struct RequestedComponent
        {
            StableTypeId type{};
            bool isWriteAccess = false;
			RequirementForComponent requirement = RequirementForComponent::Required;
        };

        struct Requests
        {
            Array<const RequestedComponent> required; // contains only required components
            Array<const RequestedComponent> optional; // contains only optional components
            Array<const RequestedComponent> subtractive; // contains only subtractive components
            Array<const RequestedComponent> writeAccess; // contains only components with write access (write access is ignored for subtractive components)
            Array<const pair<StableTypeId, RequirementForComponent>> archetypeDefining; // contains only required and subtractive components
            Array<const RequestedComponent> all; // contains all reqested components
        };

		[[nodiscard]] virtual Requests RequestedComponents() const = 0;
		[[nodiscard]] virtual StableTypeId Type() const = 0;
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem();
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const;
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem();
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const;
	};

	struct IndirectSystem : public System
	{
		[[nodiscard]] virtual IndirectSystem *AsIndirectSystem() override final;
		[[nodiscard]] virtual const IndirectSystem *AsIndirectSystem() const override final;
        virtual void ProcessMessages(const MessageStreamEntityAdded &stream) = 0;
		virtual void ProcessMessages(const MessageStreamEntityRemoved &stream) = 0;
        virtual void Update(Environment &env, MessageBuilder &messageBuilder) = 0;
	};

	struct DirectSystem : public System
	{
		[[nodiscard]] virtual DirectSystem *AsDirectSystem() override final;
		[[nodiscard]] virtual const DirectSystem *AsDirectSystem() const override final;
		virtual void Accept(Environment &env, void **array) = 0;
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