#pragma once

namespace ECSTest
{
    constexpr bool operator < (const System::RequestedComponent &left, const System::RequestedComponent &right)
    {
        return left.type < right.type;
    }

    struct _SystemHelperFuncs
    {
        template <typename T> struct GetComponentType
        {
            using type = T;
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = false;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isEntityID = false;
        };

        template <typename T> struct GetComponentType<SubtractiveComponent<T>>
        {
            using type = T;
            static constexpr bool isSubtractive = true;
            static constexpr bool isArray = false;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isEntityID = false;
        };

        template <typename T> struct GetComponentType<RequiredComponent<T>>
        {
            using type = T;
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = false;
            static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = true;
			static constexpr bool isEntityID = false;
        };

		template <typename T> struct GetComponentType<NonUnique<T>>
		{
			using type = T;
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = false;
			static constexpr bool isNonUnique = true;
            static constexpr bool isRequired = false;
			static constexpr bool isEntityID = false;
        };

        template <typename T> struct GetComponentType<Array<T>>
        {
            using type = T;
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = true;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isEntityID = false;
        };

        template <typename T> struct GetComponentType<Array<SubtractiveComponent<T>>>
        {
            using type = T;
            static constexpr bool isSubtractive = true;
            static constexpr bool isArray = true;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isEntityID = false;
        };

        template <typename T> struct GetComponentType<Array<RequiredComponent<T>>>
        {
            using type = T;
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = true;
            static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = true;
			static constexpr bool isEntityID = false;
        };

		template <typename T> struct GetComponentType<Array<NonUnique<T>>>
		{
			using type = T;
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = true;
			static constexpr bool isNonUnique = true;
            static constexpr bool isRequired = false;
			static constexpr bool isEntityID = false;
        };

        // converts void * into a properly typed T argument - a pointer or a reference
        template <typename types, uiw index> static FORCEINLINE auto ConvertArgument(void **args) -> decltype(auto)
        {
            using T = std::tuple_element_t<index, types>;
            void *arg = args[index];

            using pureType = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
            constexpr bool isRequired = GetComponentType<pureType>::isRequired;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
			constexpr bool isNonUnique = GetComponentType<pureType>::isNonUnique;

            constexpr bool isRefOrPtr = std::is_reference_v<T> || std::is_pointer_v<T>;
            static_assert((isSubtractive || isRequired) || isRefOrPtr, "Type must be either reference or pointer");
			static_assert(!isRefOrPtr || !isSubtractive, "SubtractiveComponent cannot be passed by reference or pointer");
            static_assert(!isRefOrPtr || !isRequired, "RequiredComponent cannot be passed by reference or pointer");

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
            else if constexpr (isRequired)
            {
				ASSUME(arg == nullptr);
                return RequiredComponent<componentType>{};
            }
            else if constexpr (isSubtractive)
            {
                ASSUME(arg == nullptr);
				return SubtractiveComponent<componentType>{};
            }
			else
			{
				static_assert(false, "Unrecognized argument type");
			}
        }

        // used by direct systems to convert void **array into proper function argument types
        template <typename types, typename T, uiw... Indexes> static FORCEINLINE void CallAccept(T *object, void **array, std::index_sequence<Indexes...>)
        {
			object->Accept(ConvertArgument<types, Indexes>(array)...);
        }

        // make sure the argument type is correct
        template <typename T> static constexpr void CheckArgumentType(bool &isFailed)
        {
            using pureType = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isRefOrPtr = std::is_reference_v<T> || std::is_pointer_v<T>;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
            constexpr bool isRequired = GetComponentType<pureType>::isRequired;
            constexpr bool isNonUnique = GetComponentType<pureType>::isNonUnique;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
            constexpr auto isComponent = is_base_of_v<Component, componentType>;
			constexpr bool isConst = std::is_const_v<GetComponentType<pureType>::type>;

			if constexpr (is_same_v<componentType, System::Environment>)
			{
				isFailed = true;
				if constexpr (is_same_v<pureType, System::Environment>)
				{
					static_assert(false, "Environment can't appear twice");
				}
				else
				{
					static_assert(false, "Environment can't be inside a container");
				}
			}
			else
			{
				if constexpr (isComponent == false)
				{
					isFailed = true;
					static_assert(false, "Used type is not component");
				}

				if constexpr (isConst)
				{
					isFailed = true;
					static_assert(false, "Avoid using const to mark types inside of containers, apply const to the container itself instead");
				}

				if constexpr (isNonUnique && componentType::IsUnique())
				{
					isFailed = true;
					static_assert(false, "NonUnique object used to pass unique component");
				}

				if constexpr (componentType::IsUnique() == false && (isNonUnique || isRequired || isSubtractive) == false)
				{
					isFailed = true;
					static_assert(false, "NonUnique objects must be used for non unique components");
				}

				if constexpr (is_same_v<componentType, EntityID>)
				{
					isFailed = true;
					static_assert(false, "EntityID can't appear twice");
				}

				if constexpr (isSubtractive && isArray)
				{
					isFailed = true;
					static_assert(false, "SubtractiveComponent can't be inside an Array");
				}

				if constexpr (isRequired && isArray)
				{
					isFailed = true;
					static_assert(false, "RequiredComponent can't be inside an Array");
				}

				if constexpr (isNonUnique && isArray)
				{
					isFailed = true;
					static_assert(false, "NonUnique can't be inside an Array");
				}

				if constexpr (isArray == false && (isSubtractive || isRequired || isNonUnique) == false)
				{
					isFailed = true;
					static_assert(false, "Unique components must be passed using Array");
				}
				
				if constexpr (!(isSubtractive || isRequired || isRefOrPtr))
				{
					isFailed = true;
					static_assert(false, "Components must be passed by either pointer, or by reference");
				}
			}
        }

        template <typename T> static constexpr StableTypeId ArgumentToTypeId()
        {
            using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
            using componentType = typename GetComponentType<std::remove_cv_t<TPure>>::type;
            if constexpr (is_base_of_v<_SubtractiveComponentBase, componentType>)
            {
                return componentType::ComponentType::GetTypeId();
            }
            else if constexpr (is_base_of_v<_RequiredComponentBase, componentType>)
            {
                return componentType::ComponentType::GetTypeId();
            }
			else if constexpr (is_base_of_v<_NonUniqueBase, componentType>)
			{
				return componentType::ComponentType::GetTypeId();
			}
            else if constexpr (is_base_of_v<Component, componentType>)
            {
                return componentType::GetTypeId();
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
            if constexpr (is_same_v<EntityID, componentType>)
            {
                return {{}, false, (RequirementForComponent)i8_max};
            }
            else
            {
                constexpr RequirementForComponent availability =
                    std::is_reference_v<T> ?
                    RequirementForComponent::RequiredWithData :
                    (std::is_pointer_v<T> ?
                        RequirementForComponent::Optional :
                        (is_base_of_v<_SubtractiveComponentBase, T> ?
                            RequirementForComponent::Subtractive :
                            (is_base_of_v<_RequiredComponentBase, T> ?
                                RequirementForComponent::Required :
                                (RequirementForComponent)i8_max)));
                static_assert(availability != (RequirementForComponent)i8_max, "Invalid type");
                return {componentType::GetTypeId(), !std::is_const_v<TPure>, availability};
            }
        }

		template <typename T, uiw Index> static constexpr void LocateEntityIDIndex(ui32 &index)
		{
			using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
			using componentType = typename GetComponentType<std::remove_cv_t<TPure>>::type;
			if constexpr (is_same_v<componentType, EntityID>)
			{
				static_assert(std::is_const_v<TPure>, "EntityID array must be read-only");
				static_assert(std::is_reference_v<T>, "EntityID array must be passed by reference");
				index = Index;
			}
		}

		template <typename T, uiw... Indexes> static constexpr optional<ui32> LocateEntityIDArugument(std::index_sequence<Indexes...>)
		{
			ui32 index = ui32_max;
			(LocateEntityIDIndex<std::tuple_element_t<Indexes, T>, Indexes>(index), ...);
			return index == ui32_max ? nullopt : optional<ui32>(index);
		}

		template <typename T, uiw... Indexes> static constexpr bool IsCheckingArgumentsFailed()
		{
			bool isFailed = false;
			(CheckArgumentType<std::tuple_element_t<Indexes, T>>(isFailed), ...);
			return isFailed;
		}

        template <typename T, uiw... Indexes> static constexpr std::array<System::RequestedComponent, sizeof...(Indexes)> TupleToComponentsArray(std::index_sequence<Indexes...>)
        {
			if constexpr (IsCheckingArgumentsFailed<T, Indexes...>())
			{
				return {};
			}
			else
			{
				constexpr auto typeIds = ArgumentsToTypeIds<T, Indexes...>();
				constexpr bool isAliased = IsTypesAliased(typeIds);
				static_assert(isAliased == false, "Requested type appears more than once");
				return {ArgumentToComponent<std::tuple_element_t<Indexes, T>>()...};
			}
        }

		template <typename T, uiw Index> static constexpr void LocateEnvironmentIndex(ui32 &index)
		{
			using TPure = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
			if constexpr (is_same_v<TPure, System::Environment>)
			{
				static_assert(std::is_reference_v<T>, "Environment must be passed by reference");
				index = Index;
			}
		}

		template <typename T, uiw... Indexes> static constexpr optional<ui32> LocateEnvironmentArugument(std::index_sequence<Indexes...>)
		{
			ui32 index = ui32_max;
			(LocateEnvironmentIndex<std::tuple_element_t<Indexes, T>, Indexes>(index), ...);
			return index == ui32_max ? nullopt : optional<ui32>(index);
		}

		template <uiw size, uiw requirementSize> [[nodiscard]] static constexpr uiw FindMatchingComponentsCount(const std::array<System::RequestedComponent, size> &arr, const std::array<RequirementForComponent, requirementSize> &requirements)
		{
			uiw target = 0;
			for (uiw source = 0; source < arr.size(); ++source)
			{
				for (uiw reqIndex = 0; reqIndex < requirements.size(); ++reqIndex)
				{
					if (arr[source].requirement == requirements[reqIndex])
					{
						++target;
						break;
					}
				}
			}
			return target;
		}

        template <uiw outputSize, uiw size, uiw requirementSize> [[nodiscard]] static constexpr std::array<System::RequestedComponent, outputSize> FindMatchingComponents(const std::array<System::RequestedComponent, size> &arr, const std::array<RequirementForComponent, requirementSize> &requirements)
        {
			std::array<System::RequestedComponent, outputSize> components{};
			uiw target = 0;
			for (uiw source = 0; source < arr.size(); ++source)
			{
				for (uiw reqIndex = 0; reqIndex < requirements.size(); ++reqIndex)
				{
					if (arr[source].requirement == requirements[reqIndex])
					{
						components[target++] = arr[source];
						break;
					}
				}
			}
			return components;
        }

		template <bool IsRequireWriteAccess, uiw size>[[nodiscard]] static constexpr uiw FindComponentsWithDataCount(const std::array<System::RequestedComponent, size> &arr)
		{
			uiw target = 0;
			for (uiw source = 0; source < arr.size(); ++source)
			{
				if (arr[source].requirement == RequirementForComponent::RequiredWithData || arr[source].requirement == RequirementForComponent::Optional)
				{
					if (IsRequireWriteAccess == false || arr[source].isWriteAccess)
					{
						++target;
					}
				}
			}
			return target;
		}

        template <uiw outputSize, bool IsRequireWriteAccess, uiw size> [[nodiscard]] static constexpr std::array<System::RequestedComponent, outputSize> FindComponentsWithData(const std::array<System::RequestedComponent, size> &arr)
        {
            std::array<System::RequestedComponent, outputSize> components{};
            uiw target = 0;
            for (uiw source = 0; source < arr.size(); ++source)
            {
                if (arr[source].requirement == RequirementForComponent::RequiredWithData || arr[source].requirement == RequirementForComponent::Optional)
                {
                    if (IsRequireWriteAccess == false || arr[source].isWriteAccess)
                    {
                        components[target++] = arr[source];
                    }
                }
            }
			return components;
        }

		template <uiw size> [[nodiscard]] static constexpr std::array<pair<StableTypeId, RequirementForComponent>, size> StripAccessData(const std::array<System::RequestedComponent, size> &arr)
		{
			std::array<pair<StableTypeId, RequirementForComponent>, size> output{};
			for (uiw index = 0; index < size; ++index)
			{
				output[index].first = arr[index].type;
				output[index].second = arr[index].requirement;
			}
			return output;
		}

		template <typename AcceptType> static constexpr const System::Requests &AcquireRequestedComponents()
		{
			using typesFull = typename FunctionInfo::Info<AcceptType>::args;
			static constexpr optional<ui32> environmentIndex = LocateEnvironmentArugument<typesFull>(std::make_index_sequence<std::tuple_size_v<typesFull>>());
			static constexpr ui32 environmentIndexDeref = environmentIndex.value_or(0);
			static constexpr optional<ui32> entityIDIndex = LocateEntityIDArugument<typesFull>(std::make_index_sequence<std::tuple_size_v<typesFull>>());
			static constexpr ui32 entityIDIndexDeref = entityIDIndex.value_or(0);
			using typesWithoutEnvironment = std::conditional_t<environmentIndex != nullopt, Funcs::RemoveTupleElement<environmentIndexDeref, typesFull>, typesFull>;
			static constexpr optional<ui32> entityIDIndexToRemove = LocateEntityIDArugument<typesWithoutEnvironment>(std::make_index_sequence<std::tuple_size_v<typesWithoutEnvironment>>());
			static constexpr ui32 entityIDIndexToRemoveDeref = entityIDIndexToRemove.value_or(0);
			using types = std::conditional_t<entityIDIndexToRemove != nullopt, Funcs::RemoveTupleElement<entityIDIndexToRemoveDeref, typesWithoutEnvironment>, typesWithoutEnvironment>;
			static constexpr auto converted = TupleToComponentsArray<types>(std::make_index_sequence<std::tuple_size_v<types>>());
			static constexpr auto arr = converted;
			static constexpr auto arrSorted = Funcs::SortCompileTime(arr);
			static constexpr auto requiredWithoutData = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::Required))>(arrSorted, make_array(RequirementForComponent::Required));
			static constexpr auto requiredWithData = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::RequiredWithData))>(arrSorted, make_array(RequirementForComponent::RequiredWithData));
			static constexpr auto required = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::RequiredWithData, RequirementForComponent::Required))>(arrSorted, make_array(RequirementForComponent::RequiredWithData, RequirementForComponent::Required));
			static constexpr auto requiredOrOptional = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::RequiredWithData, RequirementForComponent::Required, RequirementForComponent::Optional))>(arrSorted, make_array(RequirementForComponent::RequiredWithData, RequirementForComponent::Required, RequirementForComponent::Optional));
			static constexpr auto withData = FindComponentsWithData<FindComponentsWithDataCount<false>(arrSorted), false>(arrSorted);
			static constexpr auto optionalWithData = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::Optional))>(arrSorted, make_array(RequirementForComponent::Optional));
			static constexpr auto subtractive = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::Subtractive))>(arrSorted, make_array(RequirementForComponent::Subtractive));
			static constexpr auto writeAccess = FindComponentsWithData<FindComponentsWithDataCount<true>(arrSorted), true>(arrSorted);
			static constexpr auto archetypeDefining = FindMatchingComponents<FindMatchingComponentsCount(arrSorted, make_array(RequirementForComponent::RequiredWithData, RequirementForComponent::Required, RequirementForComponent::Subtractive))>(arrSorted, make_array(RequirementForComponent::RequiredWithData, RequirementForComponent::Required, RequirementForComponent::Subtractive));
			static constexpr auto archetypeDefiningInfoOnly = StripAccessData(archetypeDefining);
			static constexpr System::Requests requests =
			{
				ToArray(requiredWithoutData),
				ToArray(requiredWithData),
				ToArray(required),
				ToArray(requiredOrOptional),
				ToArray(withData),
				ToArray(optionalWithData),
				ToArray(subtractive),
				ToArray(writeAccess),
				ToArray(archetypeDefining),
				ToArray(arrSorted),
				ToArray(arr),
				entityIDIndex,
				environmentIndex,
				ToArray(archetypeDefiningInfoOnly)
			};
			return requests;
		}
    };

	template <typename Type, typename SystemType> struct _IndirectSystemTypeIdentifiable : public IndirectSystem, public Type
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
			
		virtual const Requests &RequestedComponents() const override final
		{
			return AcquireRequestedComponents();
		}

		static constexpr const Requests &AcquireRequestedComponents()
		{
			// saving the rusult to a local leads to ICE in MSVC 15.9.11
			static_assert(_SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>().idsArgumentNumber == nullopt, "Indirect systems can't request EntityID");
			return _SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>();
		}
	};

	template <typename Type, typename SystemType> struct _DirectSystemTypeIdentifiable : public DirectSystem, public Type
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

		static constexpr const Requests &AcquireRequestedComponents()
		{
			return _SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>();
		}

		virtual const Requests &RequestedComponents() const override final
		{
			return AcquireRequestedComponents();
		}

		virtual void AcceptUntyped(void **array) override final
		{
			using types = typename FunctionInfo::Info<decltype(&SystemType::Accept)>::args;
			static constexpr uiw count = std::tuple_size_v<types>;
			_SystemHelperFuncs::CallAccept<types>((SystemType *)this, array, std::make_index_sequence<count>());
		}
	};

	#define INDIRECT_SYSTEM(name) struct name : public _IndirectSystemTypeIdentifiable<NAME_TO_STABLE_ID(name), name>
	#define DIRECT_SYSTEM(name) struct name : public _DirectSystemTypeIdentifiable<NAME_TO_STABLE_ID(name), name>
}