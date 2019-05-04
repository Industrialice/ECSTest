#pragma once

#include "Component.hpp"
#include "MessageBuilder.hpp"
#include "LoggerWrapper.hpp"

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
            EntityIDGenerator &entityIdGenerator;
            ComponentIDGenerator &componentIdGenerator;
            MessageBuilder messageBuilder;
			LoggerWrapper logger;
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
            Array<const RequestedComponent> requiredWithData; // contains only required with data components
            Array<const RequestedComponent> withData; // contains only required with data and optional components
            Array<const RequestedComponent> optional; // contains only optional components
            Array<const RequestedComponent> subtractive; // contains only subtractive components
            Array<const RequestedComponent> writeAccess; // contains only components with write access (write access is ignored for subtractive components)
            Array<const pair<StableTypeId, RequirementForComponent>> archetypeDefining; // contains only required, required with data and subtractive components
            Array<const RequestedComponent> all; // contains all reqested components
            Array<const RequestedComponent> allOriginalOrder; // contains all reqested components in the order they are declared
            std::optional<ui32> idsArgumentNumber; // direct systems only; indicates whether EntityID array was also requested and if it was, contains its argument index
        };

		[[nodiscard]] virtual const Requests &RequestedComponents() const = 0;
		[[nodiscard]] virtual StableTypeId GetTypeId() const = 0;
        [[nodiscard]] virtual string_view GetTypeName() const = 0;
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
        [[nodiscard]] virtual StableTypeId GetTypeId() const override final
        {
            return Type::GetTypeId();
        }

        [[nodiscard]] virtual string_view GetTypeName() const override final
        {
            return Type::GetTypeName();
        }
    };

    #define INDIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentifiable<IndirectSystem, NAME_TO_STABLE_ID(name)>
	#define DIRECT_SYSTEM(name) struct name final : public _SystemTypeIdentifiable<DirectSystem, NAME_TO_STABLE_ID(name)>
}

#include "SystemHelpers.hpp"