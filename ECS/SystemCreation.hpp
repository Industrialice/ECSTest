#pragma once

namespace ECSTest
{
    constexpr bool operator < (const System::ComponentRequest &left, const System::ComponentRequest &right)
    {
        return left.type < right.type;
    }

    struct _SystemHelperFuncs
    {
        template <typename... T> struct GetComponentType
        {
			using expanded = tuple<T...>;
			using wrapped = expanded;
            using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
            static constexpr bool isSubtractive = false;
            static constexpr bool isArray = false;
			static constexpr bool isNonUnique = false;
			static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

        template <typename... T> struct GetComponentType<SubtractiveComponent<T...>>
        {
			static_assert(sizeof...(T) > 0, "Type list of SubtractiveComponent cannot be empty");
			using expanded = tuple<T...>;
			using wrapped = tuple<SubtractiveComponent<T>...>;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
            static constexpr bool isSubtractive = true;
            static constexpr bool isArray = false;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

        template <typename... T> struct GetComponentType<RequiredComponent<T...>>
        {
			static_assert(sizeof...(T) > 0, "Type list of RequiredComponent cannot be empty");
			using expanded = tuple<T...>;
			using wrapped = tuple<RequiredComponent<T>...>;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
            static constexpr bool isArray = false;
            static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = true;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

		template <typename... T> struct GetComponentType<RequiredComponentAny<T...>>
		{
			static_assert(sizeof...(T) > 0, "Type list of RequiredComponentAny cannot be empty");
			using expanded = tuple<T...>;
			using wrapped = tuple<RequiredComponentAny<T>...>;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = false;
			static constexpr bool isNonUnique = false;
			static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = true;
			static constexpr bool isEntityID = false;
		};

		template <typename... T> struct GetComponentType<NonUnique<T...>>
		{
			using expanded = tuple<T...>;
			using wrapped = expanded;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = false;
			static constexpr bool isNonUnique = true;
            static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

        template <typename... T> struct GetComponentType<Array<T...>>
        {
			using expanded = tuple<T...>;
			using wrapped = expanded;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
            static constexpr bool isArray = true;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

        template <typename... T> struct GetComponentType<Array<SubtractiveComponent<T...>>>
        {
			using expanded = tuple<T...>;
			using wrapped = expanded;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = true;
            static constexpr bool isArray = true;
			static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

        template <typename... T> struct GetComponentType<Array<RequiredComponent<T...>>>
        {
			using expanded = tuple<T...>;
			using wrapped = expanded;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
            static constexpr bool isArray = true;
            static constexpr bool isNonUnique = false;
            static constexpr bool isRequired = true;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

		template <typename... T> struct GetComponentType<Array<RequiredComponentAny<T...>>>
		{
			using expanded = tuple<T...>;
			using wrapped = tuple<RequiredComponentAny<T>...>;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = true;
			static constexpr bool isNonUnique = false;
			static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = true;
			static constexpr bool isEntityID = false;
		};

		template <typename... T> struct GetComponentType<Array<NonUnique<T...>>>
		{
			using expanded = tuple<T...>;
			using wrapped = expanded;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = true;
			static constexpr bool isNonUnique = true;
            static constexpr bool isRequired = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

		template <typename Types, uiw Index, uiw StopAt> static constexpr void CalculateIndexAdvance(uiw &index)
		{
			using T = tuple_element_t<Index, Types>;
			if constexpr (Index < StopAt && (is_reference_v<T> || is_pointer_v<T>))
			{
				++index;
			}
		}

		template <typename Types, uiw CurrentIndex, uiw... Indexes> static constexpr uiw GetArgumentIndexInArray(index_sequence<Indexes...>)
		{
			uiw index = 0;
			(CalculateIndexAdvance<Types, Indexes, CurrentIndex>(index), ...);
			return index;
		}

        // converts void * into a properly typed T argument - a pointer or a reference
        template <typename Types, uiw Index> static FORCEINLINE auto ConvertArgument(void **args) -> decltype(auto)
        {
            using T = tuple_element_t<Index, Types>;
            using pureType = remove_cv_t<remove_pointer_t<remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
            constexpr bool isRequired = GetComponentType<pureType>::isRequired;
			constexpr bool isRequiredAny = GetComponentType<pureType>::isRequiredAny;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
			constexpr bool isNonUnique = GetComponentType<pureType>::isNonUnique;

            constexpr bool isRefOrPtr = is_reference_v<T> || is_pointer_v<T>;
            static_assert((isSubtractive || isRequired || isRequiredAny) || isRefOrPtr, "Type must be either reference or pointer");
			static_assert(!isRefOrPtr || !isSubtractive, "SubtractiveComponent cannot be passed by reference or pointer");
			static_assert(!isRefOrPtr || !isRequired, "RequiredComponent cannot be passed by reference or pointer");
			static_assert(!isRefOrPtr || !isRequiredAny, "RequiredComponentAny cannot be passed by reference or pointer");

			constexpr uiw index = GetArgumentIndexInArray<Types, Index>(make_index_sequence<tuple_size_v<Types>>());

			if constexpr (is_reference_v<T>)
            {
                using bare = remove_reference_t<T>;
                static_assert(!is_pointer_v<bare>, "Type cannot be reference to pointer");
                return *(bare *)args[index];
            }
            else if constexpr (is_pointer_v<T>)
            {
                using bare = remove_pointer_t<T>;
                static_assert(!is_pointer_v<bare> && !is_reference_v<bare>, "Type cannot be pointer to reference/pointer");
                return (T)args[index];
            }
            else if constexpr (isRequired || isSubtractive || isRequiredAny)
            {
                return pureType{};
            }
			else
			{
				static_assert(false, "Unrecognized argument type");
			}
        }

        // used by direct systems to convert void **array into proper function argument types
        template <typename types, typename T, uiw... Indexes> static FORCEINLINE void CallAccept(T *object, void **array, index_sequence<Indexes...>)
        {
			object->Accept(ConvertArgument<types, Indexes>(array)...);
        }

        // make sure the argument type is correct
        template <typename T> static constexpr void CheckArgumentType(bool &isFailed)
        {
            using pureType = remove_cv_t<remove_pointer_t<remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isRefOrPtr = is_reference_v<T> || is_pointer_v<T>;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
			constexpr bool isRequired = GetComponentType<pureType>::isRequired;
			constexpr bool isRequiredAny = GetComponentType<pureType>::isRequiredAny;
            constexpr bool isNonUnique = GetComponentType<pureType>::isNonUnique;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
            constexpr auto isComponent = is_base_of_v<Component, componentType>;
			constexpr bool isConst = is_const_v<GetComponentType<pureType>::type>;

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

			if constexpr (isComponent == false)
			{
				isFailed = true;
				static_assert(false, "Used type is not component");
			}

			if constexpr (componentType::IsTag() && (isArray || isNonUnique))
			{
				isFailed = true;
				static_assert(false, "Tag components can't be inside an Array or NonUnique");
			}

			if constexpr (componentType::IsTag() == false && is_empty_v<componentType>)
			{
				static_warning(false, "Component is empty, consider using TAG_COMPONENT instead");
			}

			if constexpr (componentType::IsTag() && !is_empty_v<componentType>)
			{
				isFailed = true;
				static_assert(false, "Tag components must be empty");
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

			if constexpr (componentType::IsUnique() == false && (isNonUnique || isRequired || isRequiredAny || isSubtractive) == false)
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

			if constexpr (isRequiredAny && isArray)
			{
				isFailed = true;
				static_assert(false, "RequiredComponentAny can't be inside an Array");
			}

			if constexpr (isNonUnique && isArray)
			{
				isFailed = true;
				static_assert(false, "NonUnique can't be inside an Array");
			}

			if constexpr (isArray == false && (isSubtractive || isRequired || isRequiredAny || isNonUnique) == false)
			{
				isFailed = true;
				static_assert(false, "Unique components must be passed using Array");
			}
				
			if constexpr (!(isSubtractive || isRequired || isRequiredAny || isRefOrPtr))
			{
				isFailed = true;
				static_assert(false, "Components must be passed by either pointer, or by reference");
			}

			if constexpr (!std::is_trivially_copyable_v<componentType>)
			{
				isFailed = true;
				static_assert(false, "Components must be trivially copyable");
			}

			if constexpr (!std::is_trivially_destructible_v<componentType>)
			{
				isFailed = true;
				static_assert(false, "Components must be trivially destructible");
			}
        }

        template <typename T> static constexpr StableTypeId ArgumentToTypeId()
        {
            using TPure = remove_pointer_t<remove_reference_t<T>>;
            using componentType = typename GetComponentType<remove_cv_t<TPure>>::type;
            if constexpr (is_base_of_v<_SubtractiveComponentBase, componentType>)
            {
                return componentType::ComponentType::GetTypeId();
            }
            else if constexpr (is_base_of_v<_RequiredComponentBase, componentType>)
            {
                return componentType::ComponentType::GetTypeId();
            }
			else if constexpr (is_base_of_v<_RequiredComponentAnyBase, componentType>)
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

        template <typename T, uiw... Indexes> static constexpr array<StableTypeId, sizeof...(Indexes)> ArgumentsToTypeIds()
        {
            return {ArgumentToTypeId<tuple_element_t<Indexes, T>>()...};
        }

        // makes sure the argument type appears only once
        template <iw size> static constexpr bool IsTypesAliased(const array<StableTypeId, size> &types)
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

		template <typename T> static constexpr RequirementForComponent GetRequirementForType()
		{
			if constexpr (is_reference_v<T>)
			{
				return RequirementForComponent::RequiredWithData;
			}
			else if constexpr (is_pointer_v<T>)
			{
				return RequirementForComponent::Optional;
			}
			else if constexpr (is_base_of_v<_SubtractiveComponentBase, T>)
			{
				return RequirementForComponent::Subtractive;
			}
			else if constexpr (is_base_of_v<_RequiredComponentBase, T>)
			{
				return RequirementForComponent::Required;
			}
			else
			{
				static_assert(false, "Invalid type");
			}
		}

        // converts argument type (like Array<Component> &) into System::ComponentRequest
        template <typename T> static constexpr System::ComponentRequest ArgumentToComponent()
        {
            using TPure = remove_pointer_t<remove_reference_t<T>>;
            using componentType = typename GetComponentType<remove_cv_t<TPure>>::type;
            if constexpr (is_same_v<EntityID, componentType>)
            {
                return {{}, false, (RequirementForComponent)i8_max};
            }
            else
            {
				constexpr RequirementForComponent requirement = GetRequirementForType<T>();
                return {componentType::GetTypeId(), !is_const_v<TPure>, requirement};
            }
        }
		
		template <typename T, uiw Index> static constexpr void LocateEntityIDIndex(ui32 &filtered, ui32 &actual, bool &isLocated)
		{
			using TPure = remove_pointer_t<remove_reference_t<T>>;
			using componentType = typename GetComponentType<remove_cv_t<TPure>>::type;
			if constexpr (is_same_v<componentType, EntityID>)
			{
				static_assert(is_const_v<TPure>, "EntityID array must be read-only");
				static_assert(is_reference_v<T>, "EntityID array must be passed by reference");
				isLocated = true;
				actual = Index;
			}
			else if (GetComponentType<TPure>::isRequired == false && GetComponentType<TPure>::isSubtractive == false && isLocated == false)
			{
				++filtered;
			}
		}
		
		template <typename T, uiw... Indexes> static constexpr pair<optional<ui32>, optional<ui32>> LocateEntityIDArugument(index_sequence<Indexes...>)
		{
			ui32 filtered = 0, actual = 0;
			bool isLocated = false;
			(LocateEntityIDIndex<tuple_element_t<Indexes, T>, Indexes>(filtered, actual, isLocated), ...);
			using type = pair<optional<ui32>, optional<ui32>>;
			return isLocated ? type(filtered, actual) : type(nullopt, nullopt);
		}

		template <typename T, uiw... Indexes> static constexpr bool IsCheckingArgumentsFailed()
		{
			bool isFailed = false;
			(CheckArgumentType<tuple_element_t<Indexes, T>>(isFailed), ...);
			return isFailed;
		}

        template <typename T, uiw... Indexes> static constexpr array<System::ComponentRequest, sizeof...(Indexes)> TupleToComponentsArray(index_sequence<Indexes...>)
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
				return {ArgumentToComponent<tuple_element_t<Indexes, T>>()...};
			}
        }

		template <typename T, uiw Index> static constexpr void LocateEnvironmentIndex(ui32 &filtered, ui32 &actual, bool &isLocated)
		{
			using TPure = remove_cv_t<remove_pointer_t<remove_reference_t<T>>>;
			if constexpr (is_same_v<TPure, System::Environment>)
			{
				static_assert(is_reference_v<T>, "Environment must be passed by reference");
				isLocated = true;
				actual = Index;
			}
			else if (GetComponentType<TPure>::isRequired == false && GetComponentType<TPure>::isSubtractive == false && isLocated == false)
			{
				++filtered;
			}
		}

		template <typename T, uiw... Indexes> static constexpr pair<optional<ui32>, optional<ui32>> LocateEnvironmentArugument(index_sequence<Indexes...>)
		{
			ui32 filtered = 0, actual = 0;
			bool isLocated = false;
			(LocateEnvironmentIndex<tuple_element_t<Indexes, T>, Indexes>(filtered, actual, isLocated), ...);
			using type = pair<optional<ui32>, optional<ui32>>;
			return isLocated ? type(filtered, actual) : type(nullopt, nullopt);
		}

		template <typename T, uiw... Indexes> struct ProcessedArgumentsStruct
		{
			using withoutAny = decltype(tuple_cat(declval<conditional_t<GetComponentType<tuple_element_t<Indexes, T>>::isRequiredAny, tuple<>, tuple<tuple_element_t<Indexes, T>>>>()...));
			using onlyAny = decltype(tuple_cat(declval<conditional_t<GetComponentType<tuple_element_t<Indexes, T>>::isRequiredAny == false, tuple<>, tuple<tuple_element_t<Indexes, T>>>>()...));
		};

		template <typename T, uiw... Indexes> static constexpr ProcessedArgumentsStruct<T, Indexes...> TransformArguments(index_sequence<Indexes...>)
		{
			return {};
		}

		template <typename T, uiw... Indexes> struct UnpackArgumentsStruct
		{
			using unpacked = decltype(tuple_cat(declval<typename GetComponentType<tuple_element_t<Indexes, T>>::wrapped>()...));
		};

		template <typename T, uiw... Indexes> static constexpr UnpackArgumentsStruct<T, Indexes...> UnpackArguments(std::index_sequence<Indexes...>)
		{
			return {};
		}

		template <uiw size, uiw requirementSize> [[nodiscard]] static constexpr uiw FindMatchingComponentsCount(const array<System::ComponentRequest, size> &arr, const array<RequirementForComponent, requirementSize> &requirements)
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

        template <uiw outputSize, uiw size, uiw requirementSize> [[nodiscard]] static constexpr array<System::ComponentRequest, outputSize> FindMatchingComponents(const array<System::ComponentRequest, size> &arr, const array<RequirementForComponent, requirementSize> &requirements)
        {
			array<System::ComponentRequest, outputSize> components{};
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

		template <bool IsRequireWriteAccess, uiw size>[[nodiscard]] static constexpr uiw FindComponentsWithDataCount(const array<System::ComponentRequest, size> &arr)
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

        template <uiw outputSize, bool IsRequireWriteAccess, uiw size> [[nodiscard]] static constexpr array<System::ComponentRequest, outputSize> FindComponentsWithData(const array<System::ComponentRequest, size> &arr)
        {
            array<System::ComponentRequest, outputSize> components{};
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

		template <typename T, uiw... Indexes> static constexpr void CheckRequiredAnyGroup(bool &isFailed, index_sequence<Indexes...>)
		{
			(CheckArgumentType<tuple_element_t<Indexes, T>>(isFailed), ...);
		}

		template <typename T, uiw... Indexes> static constexpr bool IsRequiredAnyGroupsFailed()
		{
			bool isFailed = false;
			(CheckRequiredAnyGroup<typename GetComponentType<tuple_element_t<Indexes, T>>::wrapped>(isFailed, make_index_sequence<tuple_size_v<typename GetComponentType<tuple_element_t<Indexes, T>>::wrapped>>()), ...);
			return isFailed;
		}

		template <typename T, uiw Group, uiw... Indexes> static constexpr auto RequiredAnyGroupToTuple(index_sequence<Indexes...>)
		{
			return make_tuple(pair<StableTypeId, ui32>(tuple_element_t<Indexes, GetComponentType<T>::expanded>::GetTypeId(), (ui32)Group)...);
		}

		// TODO: more checks
		template <typename T, uiw... Indexes> static constexpr auto RequiredAnyToComponentsArray(index_sequence<Indexes...>)
		{
			if constexpr (IsRequiredAnyGroupsFailed<T, Indexes...>())
			{
				return array<pair<StableTypeId, ui32>, 0>{};
			}
			else
			{
				constexpr auto converted = Funcs::TupleToArray(tuple_cat(RequiredAnyGroupToTuple<tuple_element_t<Indexes, T>, Indexes>(make_index_sequence<GetComponentType<tuple_element_t<Indexes, T>>::argumentCount>())...));
				if constexpr (converted.empty())
				{
					return array<pair<StableTypeId, ui32>, 0>{};
				}
				else
				{
					return converted;
				}
			}
		}

		template <uiw NonAnySize, uiw AnySize> [[nodiscard]] static constexpr array<ArchetypeDefiningRequirement, NonAnySize + AnySize > ToArchetypeDefiningRequirement(const array<System::ComponentRequest, NonAnySize> &nonAnyComponents, const array<pair<StableTypeId, ui32>, AnySize> &anyComponents)
		{
			array<ArchetypeDefiningRequirement, NonAnySize + AnySize> output{};
			for (uiw index = 0; index < NonAnySize; ++index)
			{
				output[index] = {nonAnyComponents[index].type, (ui32)index, nonAnyComponents[index].requirement};
			}
			for (uiw index = NonAnySize, sourceIndex = 0; index < NonAnySize + AnySize; ++index, ++sourceIndex)
			{
				output[index] = {anyComponents[sourceIndex].first, anyComponents[sourceIndex].second + (ui32)NonAnySize, RequirementForComponent::Required};
			}
			return output;
		}

		template <typename AcceptType> static constexpr const System::Requests &AcquireRequestedComponents()
		{
			using rfc = RequirementForComponent;

			using typesFull = typename FunctionInfo::Info<AcceptType>::args;

			static constexpr auto environmentIndex = LocateEnvironmentArugument<typesFull>(make_index_sequence<tuple_size_v<typesFull>>());
			static constexpr auto entityIDIndex = LocateEntityIDArugument<typesFull>(make_index_sequence<tuple_size_v<typesFull>>());
			using typesWithoutEnvironment = conditional_t<environmentIndex.second != nullopt, Funcs::RemoveTupleElement<environmentIndex.second.value_or(0), typesFull>, typesFull>;

			static constexpr auto entityIDIndexToRemove = LocateEntityIDArugument<typesWithoutEnvironment>(make_index_sequence<tuple_size_v<typesWithoutEnvironment>>());
			using typesWithoutEntityID = conditional_t<entityIDIndexToRemove.second != nullopt, Funcs::RemoveTupleElement<entityIDIndexToRemove.second.value_or(0), typesWithoutEnvironment>, typesWithoutEnvironment>;

			using transformedTypes = decltype(TransformArguments<typesWithoutEntityID>(make_index_sequence<tuple_size_v<typesWithoutEntityID>>()));
			using withoutAny = typename transformedTypes::withoutAny;
			using onlyAny = typename transformedTypes::onlyAny;
			using withoutAnyUnpacked = typename decltype(UnpackArguments<withoutAny>(make_index_sequence<tuple_size_v<withoutAny>>()))::unpacked;

			static constexpr auto componentsArray = TupleToComponentsArray<withoutAnyUnpacked>(make_index_sequence<tuple_size_v<withoutAnyUnpacked>>());
			static constexpr auto sorted = Funcs::SortCompileTime(componentsArray);
			static constexpr auto requiredWithoutData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::Required))>(sorted, make_array(rfc::Required));
			static constexpr auto requiredWithData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData))>(sorted, make_array(rfc::RequiredWithData));
			static constexpr auto required = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData, rfc::Required))>(sorted, make_array(rfc::RequiredWithData, rfc::Required));
			static constexpr auto requiredOrOptional = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::Optional))>(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::Optional));
			static constexpr auto withData = FindComponentsWithData<FindComponentsWithDataCount<false>(sorted), false>(sorted);
			static constexpr auto optionalWithData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::Optional))>(sorted, make_array(rfc::Optional));
			static constexpr auto subtractive = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::Subtractive))>(sorted, make_array(rfc::Subtractive));
			static constexpr auto writeAccess = FindComponentsWithData<FindComponentsWithDataCount<true>(sorted), true>(sorted);
			static constexpr auto argumentPassingOrder = FindMatchingComponents<FindMatchingComponentsCount(componentsArray, make_array(rfc::RequiredWithData, rfc::Optional))>(componentsArray, make_array(rfc::RequiredWithData, rfc::Optional));
			
			static constexpr auto archetypeDefining = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::Subtractive))>(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::Subtractive));
			static constexpr auto requiredAnyArguments = RequiredAnyToComponentsArray<onlyAny>(make_index_sequence<tuple_size_v<onlyAny>>());
			static constexpr auto archetypeDefiningInfoOnly = ToArchetypeDefiningRequirement(archetypeDefining, requiredAnyArguments);

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
				ToArray(sorted),
				ToArray(argumentPassingOrder),
				entityIDIndex.first,
				environmentIndex.first,
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
			static_assert(_SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>().idsArgumentIndex == nullopt, "Indirect systems can't request EntityID");
			static_assert(_SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>().environmentArgumentIndex == nullopt, "Indirect systems don't need to request Environment");
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
			static constexpr uiw count = tuple_size_v<types>;
			_SystemHelperFuncs::CallAccept<types>((SystemType *)this, array, make_index_sequence<count>());
		}
	};

	#define INDIRECT_SYSTEM(name) struct name : public _IndirectSystemTypeIdentifiable<NAME_TO_STABLE_ID(name), name>
	#define DIRECT_SYSTEM(name) struct name : public _DirectSystemTypeIdentifiable<NAME_TO_STABLE_ID(name), name>
}