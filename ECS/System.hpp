#pragma once

#include "Component.hpp"
#include "MessageBuilder.hpp"
#include "LoggerWrapper.hpp"
#include "IKeyController.hpp"

namespace ECSTest
{
    class System
    {
        shared_ptr<IKeyController> _keyController{};

    public:
        struct Environment
        {
			const f32 timeSinceLastFrame;
            const ui32 frameNumber;
			const TimeDifference timeSinceStarted;
            const StableTypeId targetSystem; // useful when the same class used to implement different systems
            EntityIDGenerator &entityIdGenerator;
            ComponentIDGenerator &componentIdGenerator;
            MessageBuilder &messageBuilder;
			LoggerWrapper logger;
            IKeyController *keyController;
        };

		struct ComponentRequest
		{
			StableTypeId type{};
			bool isWriteAccess = false;
			RequirementForComponent requirement = RequirementForComponent::Required;
		};

        struct Requests
        {
            Array<const ComponentRequest> requiredWithoutData; // contains only required components that don't require data
            Array<const ComponentRequest> requiredWithData; // contains only required with data components
			Array<const ComponentRequest> required; // contains only required components, both with and without data
			Array<const ComponentRequest> requiredOrOptional; // contains only required and optional components, both with and without data
			Array<const ComponentRequest> withData; // contains only required with data and optional components
            Array<const ComponentRequest> optionalWithData; // contains only optional components
            Array<const ComponentRequest> subtractive; // contains only subtractive components
            Array<const ComponentRequest> writeAccess; // contains only components with write access (write access is ignored for subtractive components)
            Array<const ComponentRequest> all; // contains all requested components except RequiredComponentAny, the order is not preserved
            Array<const ComponentRequest> argumentPassingOrder; // contains components with data in the order they are declared
            optional<ui32> idsArgumentIndex; // direct systems only; indicates whether EntityID array was also requested and if it was, contains its argument index, SubtractiveComponent, RequiredComponent or RequiredComponentAny are not accounted for
			optional<ui32> environmentArgumentIndex; // same as for idsArgumentIndex, but for Environment variable
			Array<const ArchetypeDefiningRequirement> archetypeDefiningInfoOnly; // contains elements from archetypeDefining, but without the access information
        };

		[[nodiscard]] virtual const Requests &RequestedComponents() const = 0;
		[[nodiscard]] virtual StableTypeId GetTypeId() const = 0;
        [[nodiscard]] virtual string_view GetTypeName() const = 0;
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem();
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const;
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem();
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const;
        [[nodiscard]] IKeyController *GetKeyController();
        [[nodiscard]] const IKeyController *GetKeyController() const;
        void SetKeyController(const shared_ptr<IKeyController> &controller);
        virtual bool ControlInput(Environment &env, const ControlAction &input) { SOFTBREAK; return false; }
        virtual void OnCreate(Environment &env) {}
        virtual void OnInitialized(Environment &env) {}
        virtual void OnDestroy(Environment &env) {}
	};

	struct IndirectSystem : public System
	{
		[[nodiscard]] virtual IndirectSystem *AsIndirectSystem() override final;
		[[nodiscard]] virtual const IndirectSystem *AsIndirectSystem() const override final;
        virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) { SOFTBREAK; }
        virtual void Update(Environment &env) { SOFTBREAK; }
	};

	struct DirectSystem : public System
	{
		[[nodiscard]] virtual DirectSystem *AsDirectSystem() override final;
		[[nodiscard]] virtual const DirectSystem *AsDirectSystem() const override final;
		virtual void AcceptUntyped(void **array) = 0;
	};
}