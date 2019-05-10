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

        struct RequestedComponent
        {
            StableTypeId type{};
            bool isWriteAccess = false;
			RequirementForComponent requirement = RequirementForComponent::Required;
        };

        struct Requests
        {
            Array<const RequestedComponent> requiredWithoutData; // contains only required components that don't require data
            Array<const RequestedComponent> requiredWithData; // contains only required with data components
			Array<const RequestedComponent> required; // contains only required components, both with and without data
			Array<const RequestedComponent> requiredOrOptional; // contains only required and optional components, both with and without data
            Array<const RequestedComponent> withData; // contains only required with data and optional components
            Array<const RequestedComponent> optionalWithData; // contains only optional components
            Array<const RequestedComponent> subtractive; // contains only subtractive components
            Array<const RequestedComponent> writeAccess; // contains only components with write access (write access is ignored for subtractive components)
            Array<const RequestedComponent> archetypeDefining; // contains only required and subtractive components
            Array<const RequestedComponent> all; // contains all requested components
            Array<const RequestedComponent> allOriginalOrder; // contains all requested components in the order they are declared
            optional<ui32> idsArgumentNumber; // direct systems only; indicates whether EntityID array was also requested and if it was, contains its argument index
			optional<ui32> environmentArgumentNumber; // direct systems only; indicates whether Environment was also requested and if it was, contains its argument index
			Array<const pair<StableTypeId, RequirementForComponent>> archetypeDefiningInfoOnly; // contains elements from archetypeDefining, but without the access information
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