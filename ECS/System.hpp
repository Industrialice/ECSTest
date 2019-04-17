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
			const f32 timeSinceLastFrame;
            const ui32 frameNumber;
			const TimeDifference timeSinceStarted;
            EntityIDGenerator &idGenerator;
            MessageBuilder messageBuilder;
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
            Array<const RequestedComponent> allOriginalOrder; // contains all reqested components in the order they are declared
        };

		[[nodiscard]] virtual Requests RequestedComponents() const = 0;
		[[nodiscard]] virtual StableTypeId Type() const = 0;
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem();
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const;
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem();
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const;
        virtual void OnCreate(Environment &env) {}
        virtual void OnDestroy(Environment &env) {}
	};

	struct IndirectSystem : public System
	{
		[[nodiscard]] virtual IndirectSystem *AsIndirectSystem() override final;
		[[nodiscard]] virtual const IndirectSystem *AsIndirectSystem() const override final;
        virtual void ProcessMessages(const MessageStreamEntityAdded &stream) = 0;
        virtual void ProcessMessages(const MessageStreamComponentAdded &stream) = 0;
        virtual void ProcessMessages(const MessageStreamComponentChanged &stream) = 0;
        virtual void ProcessMessages(const MessageStreamComponentRemoved &stream) = 0;
		virtual void ProcessMessages(const MessageStreamEntityRemoved &stream) = 0;
        virtual void Update(Environment &env) = 0;
	};

	struct DirectSystem : public System
	{
		[[nodiscard]] virtual DirectSystem *AsDirectSystem() override final;
		[[nodiscard]] virtual const DirectSystem *AsDirectSystem() const override final;
		virtual void Accept(Environment &env, void **array) = 0;
	};

    template <typename BaseSystem, typename Type> struct _SystemTypeIdentifiable : public BaseSystem, public Type
    {
    public:
        using Type::GetTypeId;

        [[nodiscard]] virtual StableTypeId Type() const override final
        {
            return GetTypeId();
        }
    };

    #ifdef USE_ID_NAMES
        #define INDIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentifiable<IndirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>>
    #else
        #define INDIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentifiable<IndirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>>
    #endif

    #ifdef USE_ID_NAMES
        #define DIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentifiable<DirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
            CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>>
    #else
        #define DIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentifiable<DirectSystem, StableTypeIdentifiable<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>>
    #endif
}

#include "SystemHelpers.hpp"