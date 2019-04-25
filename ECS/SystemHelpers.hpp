#pragma once

namespace ECSTest
{
    constexpr bool operator < (const System::RequestedComponent &left, const System::RequestedComponent &right)
    {
        return std::tie(left.isWriteAccess, left.requirement, left.type) < std::tie(right.isWriteAccess, right.requirement, right.type);
    }

    struct _SystemHelperFuncs
    {
        template <typename T> struct GetComponentType
        {
            using type = T;
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = false;
        };

        template <typename T> struct GetComponentType<SubtractiveComponent<T>>
        {
            using type = T;
            static constexpr bool isSubtractive = true;
            static constexpr bool isArray = false;
        };

        template <typename T> struct GetComponentType<Array<T>>
        {
            using type = T;
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = true;
        };

        template <typename T> struct GetComponentType<Array<SubtractiveComponent<T>>>
        {
            using type = T;
            static constexpr bool isSubtractive = true;
            static constexpr bool isArray = true;
        };

        // converts void * into a properly typed T argument - a pointer or a reference
        template <typename types, uiw index> static FORCEINLINE auto ConvertArgument(void **args) -> decltype(auto)
        {
            using T = std::tuple_element_t<index, types>;
            void *arg = args[index];

            using pureType = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
            constexpr bool isArray = GetComponentType<pureType>::isArray;

            constexpr bool isRefOrPtr = std::is_reference_v<T> || std::is_pointer_v<T>;
            static_assert(isSubtractive || isRefOrPtr, "Type must be either reference or pointer");
            static_assert(!isSubtractive || !isRefOrPtr, "Subtractive component cannot be passed by reference or pointer");
            static_assert(isSubtractive || isArray, "Non-subtractive components must be passed using Array<T>");
            static_assert(!isSubtractive || !isArray, "Subtractive component cannot be inside an array");

            if constexpr (std::is_reference_v<T>)
            {
                using bare = std::remove_reference_t<T>;
                static_assert(!std::is_pointer_v<bare>, "Type cannot be reference to pointer");
                return *(bare *)arg;
            }
            else if constexpr (std::is_pointer_v<T>)
            {
                using bare = std::remove_pointer_t<T>;
                static_assert(!std::is_pointer_v<bare> && !std::is_reference_v<bare>, "Type cannot be pointer to reference/pointer");
                return (T)arg;
            }
            else
            {
                ASSUME(arg == nullptr);
                return nullptr;
            }
        }

        // used by direct systems to convert void **array into proper function argument types
        template <auto Method, typename types, typename T, uiw... Indexes> static FORCEINLINE void CallAccept(T *object, System::Environment &env, void **array, std::index_sequence<Indexes...>)
        {
            (object->*Method)(env, ConvertArgument<types, Indexes>(array)...);
        }

        // make sure the argument type is correct
        template <typename T> static constexpr void CheckArgumentType()
        {
            using pureType = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
            constexpr auto isComponent = std::is_base_of_v<Component, componentType>;
            constexpr auto isEntityID = std::is_same_v<EntityID, componentType>;
            static_assert(isSubtractive != isArray, "SubtractiveComponent can't be inside an Array");
            static_assert(isComponent || (isSubtractive == false), "SubtractiveComponent used with a non-component type");
            static_assert(isSubtractive || isComponent || isEntityID, "Invalid argument type, must be either Component, SubtractiveComponent, or EntityID");
        }

        template <typename T> static constexpr StableTypeId ArgumentToTypeId()
        {
            using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
            using componentType = typename GetComponentType<std::remove_cv_t<TPure>>::type;
            if constexpr (std::is_base_of_v<_SubtractiveComponentBase, componentType>)
            {
                return componentType::ComponentType::GetTypeId();
            }
            else if constexpr (std::is_base_of_v<Component, componentType>)
            {
                return componentType::GetTypeId();
            }
            else if constexpr (std::is_same_v<EntityID, componentType>)
            {
                static_assert(std::is_const_v<TPure>, "EntityID array must be read-only");
                return NAME_TO_STABLE_ID("EntityID")::GetTypeId();
            }
            else
            {
                static_assert(false, "Failed to convert argument type to id");
                return {};
            }
        }

        template <typename T, uiw... Indexes> static constexpr std::array<StableTypeId, sizeof...(Indexes)> ArgumentsToTypeIds()
        {
            return {ArgumentToTypeId<std::tuple_element_t<Indexes, T>>()...};
        }

        // makes sure the argument type appears only once
        template <iw size> static constexpr bool IsTypesAliased(const std::array<StableTypeId, size> &types)
        {
            for (iw i = 0; i < size - 1; ++i)
            {
                for (iw j = i + 1; j < size; ++j)
                {
                    if (types[i] == types[j])
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        // converts argument type (like Array<Component> &) into System::RequestedComponent
        template <typename T> static constexpr System::RequestedComponent ArgumentToComponent()
        {
            using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
            using componentType = typename GetComponentType<std::remove_cv_t<TPure>>::type;
            if constexpr (std::is_same_v<EntityID, componentType>)
            {
                return {{}, false, (RequirementForComponent)i8_max};
            }
            else
            {
                constexpr RequirementForComponent availability =
                    std::is_reference_v<T> ?
                    RequirementForComponent::Required :
                    (std::is_pointer_v<T> ?
                        RequirementForComponent::Optional :
                        (std::is_base_of_v<_SubtractiveComponentBase, T> ?
                            RequirementForComponent::Subtractive :
                            (RequirementForComponent)i8_max));
                static_assert(availability != (RequirementForComponent)i8_max, "Invalid type");
                return {componentType::GetTypeId(), !std::is_const_v<TPure>, availability};
            }
        }

        template <uiw size> static constexpr optional<ui32> FindEntityIDIndex(const std::array<StableTypeId, size> &types)
        {
            for (uiw index = 0; index < size; ++index)
            {
                if (types[index] == NAME_TO_STABLE_ID("EntityID")::GetTypeId())
                {
                    return index;
                }
            }
            return nullopt;
        }

        template <typename T, bool IsEntityIDIndexValid, uiw... Indexes> static constexpr auto ConvertToComponents()
        {
            constexpr uiw count = sizeof...(Indexes);
            std::array<System::RequestedComponent, count> converted = {ArgumentToComponent<std::tuple_element_t<Indexes, T>>()...};
            if constexpr (IsEntityIDIndexValid)
            {
                std::array<System::RequestedComponent, count - 1> arr{};
                for (uiw src = 0, dst = 0; src < count; ++src)
                {
                    if (converted[src].requirement != (RequirementForComponent)i8_max)
                    {
                        arr[dst++] = converted[src];
                    }
                }
                return arr;
            }
            else
            {
                return converted;
            }
        }

        template <typename T, uiw... Indexes> static constexpr auto TupleToComponentsArray(std::index_sequence<Indexes...>)
        {
            (CheckArgumentType<std::tuple_element_t<Indexes, T>>(), ...);
            constexpr auto typeIds = ArgumentsToTypeIds<T, Indexes...>();
            constexpr bool isAliased = IsTypesAliased(typeIds);
            static_assert(isAliased == false, "Requested type appears more than once");
            constexpr auto entityIDIndex = FindEntityIDIndex(typeIds);
            return std::pair{ConvertToComponents<T, entityIDIndex != nullopt, Indexes...>(), entityIDIndex};
        }

        template <uiw size>[[nodiscard]] static constexpr pair<std::array<System::RequestedComponent, size>, uiw> FindMatchingComponents(const std::array<System::RequestedComponent, size> &arr, RequirementForComponent requirement)
        {
            std::array<System::RequestedComponent, size> components{};
            uiw target = 0;
            for (uiw source = 0; source < arr.size(); ++source)
            {
                if (arr[source].requirement == requirement)
                {
                    components[target++] = arr[source];
                }
            }
            return {components, target};
        }

        template <uiw size>[[nodiscard]] static constexpr pair<std::array<System::RequestedComponent, size>, uiw> FindComponentsWithWriteAccess(const std::array<System::RequestedComponent, size> &arr)
        {
            std::array<System::RequestedComponent, size> components{};
            uiw target = 0;
            for (uiw source = 0; source < arr.size(); ++source)
            {
                if (arr[source].requirement != RequirementForComponent::Subtractive && arr[source].isWriteAccess)
                {
                    components[target++] = arr[source];
                }
            }
            return {components, target};
        }

        template <uiw size>[[nodiscard]] static constexpr pair<std::array<pair<StableTypeId, RequirementForComponent>, size>, uiw> FindArchetypeDefiningComponents(const std::array<System::RequestedComponent, size> &arr)
        {
            std::array<pair<StableTypeId, RequirementForComponent>, size> components{};
            uiw target = 0;
            for (uiw source = 0; source < arr.size(); ++source)
            {
                if (arr[source].requirement == RequirementForComponent::Required || arr[source].requirement == RequirementForComponent::Subtractive)
                {
                    components[target].first = arr[source].type;
                    components[target].second = arr[source].requirement;
                    ++target;
                }
            }
            return {components, target};
        }
    };
}

// TODO: optimize it so std::arrays use only as much space as they actually need

#define DIRECT_ACCEPT_COMPONENTS(...) \
    virtual const Requests &RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		struct Local { static void Temp(__VA_ARGS__); }; \
		using types = typename FunctionInfo::Info<decltype(Local::Temp)>::args; \
        static constexpr auto converted = _SystemHelperFuncs::TupleToComponentsArray<types>(std::make_index_sequence<std::tuple_size_v<types>>()); \
        static constexpr auto arr = converted.first; \
        static constexpr auto arrSorted = Funcs::SortCompileTime(arr); \
        static constexpr auto required = _SystemHelperFuncs::FindMatchingComponents(arrSorted, RequirementForComponent::Required); \
        static constexpr auto opt = _SystemHelperFuncs::FindMatchingComponents(arrSorted, RequirementForComponent::Optional); \
        static constexpr auto subtractive = _SystemHelperFuncs::FindMatchingComponents(arrSorted, RequirementForComponent::Subtractive); \
        static constexpr auto writeAccess = _SystemHelperFuncs::FindComponentsWithWriteAccess(arrSorted); \
        static constexpr auto archetypeDefining = _SystemHelperFuncs::FindArchetypeDefiningComponents(arrSorted); \
        static constexpr Requests requests = \
        { \
            ToArray(required.first.data(), required.second), \
            ToArray(opt.first.data(), opt.second), \
            ToArray(subtractive.first.data(), subtractive.second), \
            ToArray(writeAccess.first.data(), writeAccess.second), \
            ToArray(archetypeDefining.first.data(), archetypeDefining.second), \
            ToArray(arrSorted), \
            ToArray(arr), \
            converted.second \
        }; \
        return requests; \
    } \
    \
    virtual void Accept(Environment &env, void **array) override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		struct Local { static void Temp(__VA_ARGS__); }; \
		using types = typename FunctionInfo::Info<decltype(Local::Temp)>::args; \
		static constexpr uiw count = std::tuple_size_v<types>; \
        _SystemHelperFuncs::CallAccept<&thisType::Update, types>(this, env, array, std::make_index_sequence<count>()); \
    } \
    void Update(Environment &env, __VA_ARGS__)

#define INDIRECT_ACCEPT_COMPONENTS(...) \
    virtual const Requests &RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		struct Local { static void Temp(__VA_ARGS__); }; \
		using types = typename FunctionInfo::Info<decltype(Local::Temp)>::args; \
        static constexpr auto converted = _SystemHelperFuncs::TupleToComponentsArray<types>(std::make_index_sequence<std::tuple_size_v<types>>()); \
        static_assert(converted.second == nullopt, "Indirect systems can't request EntityID"); \
        static constexpr auto arr = converted.first; \
        static constexpr auto arrSorted = Funcs::SortCompileTime(arr); \
        static constexpr auto required = _SystemHelperFuncs::FindMatchingComponents(arrSorted, RequirementForComponent::Required); \
        static constexpr auto opt = _SystemHelperFuncs::FindMatchingComponents(arrSorted, RequirementForComponent::Optional); \
        static constexpr auto subtractive = _SystemHelperFuncs::FindMatchingComponents(arrSorted, RequirementForComponent::Subtractive); \
        static constexpr auto writeAccess = _SystemHelperFuncs::FindComponentsWithWriteAccess(arrSorted); \
        static constexpr auto archetypeDefining = _SystemHelperFuncs::FindArchetypeDefiningComponents(arrSorted); \
        static constexpr Requests requests = \
        { \
            ToArray(required.first.data(), required.second), \
            ToArray(opt.first.data(), opt.second), \
            ToArray(subtractive.first.data(), subtractive.second), \
            ToArray(writeAccess.first.data(), writeAccess.second), \
            ToArray(archetypeDefining.first.data(), archetypeDefining.second), \
            ToArray(arrSorted), \
            ToArray(arr), \
            nullopt \
        }; \
        return requests; \
    } \
    \
	virtual void ProcessMessages(const MessageStreamEntityAdded &stream) override; \
    virtual void ProcessMessages(const MessageStreamComponentAdded &stream) override; \
    virtual void ProcessMessages(const MessageStreamComponentChanged &stream) override; \
    virtual void ProcessMessages(const MessageStreamComponentRemoved &stream) override; \
	virtual void ProcessMessages(const MessageStreamEntityRemoved &stream) override; \
    virtual void Update(Environment &env) override