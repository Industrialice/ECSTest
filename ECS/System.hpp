#pragma once

#include "Component.hpp"
#include "MessageBuilder.hpp"
#include "LoggerWrapper.hpp"
#include "IKeyController.hpp"
#include "AssetsManager.hpp"

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
            const TypeId targetSystem; // useful when the same class used to implement different systems
            EntityIDGenerator &entityIdGenerator;
            ComponentIDGenerator &componentIdGenerator;
            MessageBuilder &messageBuilder;
			LoggerWrapper logger;
            IKeyController *keyController;
			AssetsManager &assetsManager;
        };

		struct ComponentRequest
		{
			TypeId type{};
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
			Array<const ComponentRequest> optional; // contains only optional components
            Array<const ComponentRequest> optionalWithData; // contains only optional components with data
            Array<const ComponentRequest> subtractive; // contains only subtractive components
            Array<const ComponentRequest> writeAccess; // contains only components with write access (write access is ignored for subtractive components)
            Array<const ComponentRequest> all; // contains all requested components except RequiredComponentAny, the order is not preserved
            Array<const ComponentRequest> argumentPassingOrder; // contains components with data in the order they are declared
            std::optional<ui32> entityIDIndex; // direct systems only; indicates whether EntityID array was also requested and if it was, contains its argument index, SubtractiveComponent, RequiredComponent or RequiredComponentAny are not accounted for
			std::optional<ui32> environmentIndex; // same as for entityIDIndex, but for Environment variable
			Array<const ArchetypeDefiningRequirement> archetypeDefiningInfoOnly; // contains elements from archetypeDefining, but without the access information
        };

		virtual ~System() = default;
		[[nodiscard]] virtual const Requests &RequestedComponents() const = 0;
		[[nodiscard]] virtual TypeId GetTypeId() const = 0;
        [[nodiscard]] virtual string_view GetTypeName() const = 0;
		[[nodiscard]] virtual struct BaseIndirectSystem *AsIndirectSystem();
		[[nodiscard]] virtual const struct BaseIndirectSystem *AsIndirectSystem() const;
		[[nodiscard]] virtual struct BaseDirectSystem *AsDirectSystem();
		[[nodiscard]] virtual const struct BaseDirectSystem *AsDirectSystem() const;
        [[nodiscard]] IKeyController *GetKeyController();
        [[nodiscard]] const IKeyController *GetKeyController() const;
        void SetKeyController(const shared_ptr<IKeyController> &controller);
		[[nodiscard]] virtual bool ControlInput(Environment &env, const ControlAction &input) { SOFTBREAK; return false; }
        virtual void OnCreate(Environment &env) {}
        virtual void OnInitialized(Environment &env) {}
        virtual void OnDestroy(Environment &env) {}
	};

	struct BaseIndirectSystem : public System
	{
		[[nodiscard]] virtual BaseIndirectSystem *AsIndirectSystem() override final;
		[[nodiscard]] virtual const BaseIndirectSystem *AsIndirectSystem() const override final;
        virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) { SOFTBREAK; }
        virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) { SOFTBREAK; }
        virtual void Update(Environment &env) { SOFTBREAK; }
	};

	struct BaseDirectSystem : public System
	{
		[[nodiscard]] virtual BaseDirectSystem *AsDirectSystem() override final;
		[[nodiscard]] virtual const BaseDirectSystem *AsDirectSystem() const override final;
		virtual void AcceptUntyped(void **array) = 0;
	};
}