#pragma once

#include "System.hpp"

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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

		template <typename... T> struct GetComponentType<OptionalComponent<T...>>
		{
			static_assert(sizeof...(T) > 0, "Type list of OptionalComponent cannot be empty");
			using expanded = tuple<T...>;
			using wrapped = tuple<OptionalComponent<T>...>;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = false;
			static constexpr bool isNonUnique = false;
			static constexpr bool isRequired = false;
			static constexpr bool isOptional = true;
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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
			static constexpr bool isRequiredAny = false;
			static constexpr bool isEntityID = false;
        };

		template <typename... T> struct GetComponentType<Array<OptionalComponent<T...>>>
		{
			using expanded = tuple<T...>;
			using wrapped = expanded;
			using type = tuple_element_t<0, expanded>;
			static constexpr uiw argumentCount = sizeof...(T);
			static constexpr bool isSubtractive = false;
			static constexpr bool isArray = true;
			static constexpr bool isNonUnique = false;
			static constexpr bool isRequired = false;
			static constexpr bool isOptional = true;
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
			static constexpr bool isOptional = false;
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
			static constexpr bool isOptional = false;
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

		template <typename Types, uiw CurrentIndex, uiw... Indexes> [[nodiscard]] static constexpr uiw GetArgumentIndexInArray(index_sequence<Indexes...>)
		{
			uiw index = 0;
			(CalculateIndexAdvance<Types, Indexes, CurrentIndex>(index), ...);
			return index;
		}

        // converts void * into a properly typed T argument - a pointer or a reference
        template <typename Types, uiw Index> [[nodiscard]] static FORCEINLINE auto ConvertArgument(void **args) -> decltype(auto)
        {
            using T = tuple_element_t<Index, Types>;
            using pureType = remove_cv_t<remove_pointer_t<remove_reference_t<T>>>;
            using componentType = typename GetComponentType<pureType>::type;
            constexpr bool isSubtractive = GetComponentType<pureType>::isSubtractive;
            constexpr bool isRequired = GetComponentType<pureType>::isRequired;
			constexpr bool isOptional = GetComponentType<pureType>::isOptional;
			constexpr bool isRequiredAny = GetComponentType<pureType>::isRequiredAny;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
			constexpr bool isNonUnique = GetComponentType<pureType>::isNonUnique;

            constexpr bool isRefOrPtr = is_reference_v<T> || is_pointer_v<T>;
            static_assert((isSubtractive || isRequired || isRequiredAny || isOptional) || isRefOrPtr, "Type must be either reference or pointer");
			static_assert(!isRefOrPtr || !isSubtractive, "SubtractiveComponent cannot be passed by reference or pointer");
			static_assert(!isRefOrPtr || !isRequired, "RequiredComponent cannot be passed by reference or pointer");
			static_assert(!isRefOrPtr || !isOptional, "OptionalComponent cannot be passed by reference or pointer");
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
            else if constexpr (isRequired || isSubtractive || isOptional || isRequiredAny)
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
			constexpr bool isOptional = GetComponentType<pureType>::isOptional;
			constexpr bool isRequiredAny = GetComponentType<pureType>::isRequiredAny;
            constexpr bool isNonUnique = GetComponentType<pureType>::isNonUnique;
            constexpr bool isArray = GetComponentType<pureType>::isArray;
            constexpr auto isComponent = is_base_of_v<_BaseComponentClass, componentType>;
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

			if constexpr (componentType::IsUnique() == false && (isNonUnique || isRequired || isOptional || isRequiredAny || isSubtractive) == false)
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

			if constexpr (isOptional && isArray)
			{
				isFailed = true;
				static_assert(false, "OptionalComponent can't be inside an Array");
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

			if constexpr (isArray == false && (isSubtractive || isRequired || isOptional || isRequiredAny || isNonUnique) == false)
			{
				isFailed = true;
				static_assert(false, "Unique components must be passed using Array");
			}
				
			if constexpr (!(isSubtractive || isRequired || isOptional || isRequiredAny || isRefOrPtr))
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

        template <typename T> [[nodiscard]] static constexpr TypeId ArgumentToTypeId()
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
			else if constexpr (is_base_of_v<_OptionalComponentBase, componentType>)
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
            else if constexpr (is_base_of_v<_BaseComponentClass, componentType>)
            {
                return componentType::GetTypeId();
            }
            else
            {
                static_assert(false, "Failed to convert argument type to id");
                return {};
            }
        }

        template <typename T, uiw... Indexes> [[nodiscard]] static constexpr array<TypeId, sizeof...(Indexes)> ArgumentsToTypeIds()
        {
            return {ArgumentToTypeId<tuple_element_t<Indexes, T>>()...};
        }

        // makes sure the argument type appears only once
        template <iw size> [[nodiscard]] static constexpr bool IsTypesAliased(const array<TypeId, size> &types)
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

		template <typename T> [[nodiscard]] static constexpr RequirementForComponent GetRequirementForType()
		{
			if constexpr (is_reference_v<T>)
			{
				return RequirementForComponent::RequiredWithData;
			}
			else if constexpr (is_pointer_v<T>)
			{
				return RequirementForComponent::OptionalWithData;
			}
			else if constexpr (is_base_of_v<_SubtractiveComponentBase, T>)
			{
				return RequirementForComponent::Subtractive;
			}
			else if constexpr (is_base_of_v<_RequiredComponentBase, T>)
			{
				return RequirementForComponent::Required;
			}
			else if constexpr (is_base_of_v<_OptionalComponentBase, T>)
			{
				return RequirementForComponent::Optional;
			}
			else
			{
				static_assert(false, "Invalid type");
			}
		}

        // converts argument type (like Array<Component> &) into System::ComponentRequest
        template <typename T> [[nodiscard]] static constexpr System::ComponentRequest ArgumentToComponent()
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
		
		template <typename T, uiw Index> [[nodiscard]] static constexpr void LocateEntityIDIndex(ui32 &filtered, ui32 &actual, bool &isLocated)
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
			else if (GetComponentType<TPure>::isRequired == false && GetComponentType<TPure>::isSubtractive == false && GetComponentType<TPure>::isOptional == false && isLocated == false)
			{
				++filtered;
			}
		}
		
		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr pair<optional<ui32>, optional<ui32>> LocateEntityIDArugument(index_sequence<Indexes...>)
		{
			ui32 filtered = 0, actual = 0;
			bool isLocated = false;
			(LocateEntityIDIndex<tuple_element_t<Indexes, T>, Indexes>(filtered, actual, isLocated), ...);
			using type = pair<optional<ui32>, optional<ui32>>;
			return isLocated ? type(filtered, actual) : type(nullopt, nullopt);
		}

		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr bool IsCheckingArgumentsFailed()
		{
			bool isFailed = false;
			(CheckArgumentType<tuple_element_t<Indexes, T>>(isFailed), ...);
			return isFailed;
		}

        template <typename T, uiw... Indexes> [[nodiscard]] static constexpr array<System::ComponentRequest, sizeof...(Indexes)> TupleToComponentsArray(index_sequence<Indexes...>)
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

		template <typename T, uiw Index> [[nodiscard]] static constexpr void LocateEnvironmentIndex(ui32 &filtered, ui32 &actual, bool &isLocated)
		{
			using TPure = remove_cv_t<remove_pointer_t<remove_reference_t<T>>>;
			if constexpr (is_same_v<TPure, System::Environment>)
			{
				static_assert(is_reference_v<T>, "Environment must be passed by reference");
				isLocated = true;
				actual = Index;
			}
			else if (GetComponentType<TPure>::isRequired == false && GetComponentType<TPure>::isSubtractive == false && GetComponentType<TPure>::isOptional == false && isLocated == false)
			{
				++filtered;
			}
		}

		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr pair<optional<ui32>, optional<ui32>> LocateEnvironmentArugument(index_sequence<Indexes...>)
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

		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr ProcessedArgumentsStruct<T, Indexes...> TransformArguments(index_sequence<Indexes...>)
		{
			return {};
		}

		template <typename T, uiw... Indexes> struct UnpackArgumentsStruct
		{
			using unpacked = decltype(tuple_cat(declval<typename GetComponentType<tuple_element_t<Indexes, T>>::wrapped>()...));
		};

		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr UnpackArgumentsStruct<T, Indexes...> UnpackArguments(std::index_sequence<Indexes...>)
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

		template <bool IsRequireWriteAccess, uiw size> [[nodiscard]] static constexpr uiw FindComponentsWithDataCount(const array<System::ComponentRequest, size> &arr)
		{
			uiw target = 0;
			for (uiw source = 0; source < arr.size(); ++source)
			{
				if (arr[source].requirement == RequirementForComponent::RequiredWithData || arr[source].requirement == RequirementForComponent::OptionalWithData)
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
                if (arr[source].requirement == RequirementForComponent::RequiredWithData || arr[source].requirement == RequirementForComponent::OptionalWithData)
                {
                    if (IsRequireWriteAccess == false || arr[source].isWriteAccess)
                    {
                        components[target++] = arr[source];
                    }
                }
            }
			return components;
        }

		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr void CheckRequiredAnyGroup(bool &isFailed, index_sequence<Indexes...>)
		{
			(CheckArgumentType<tuple_element_t<Indexes, T>>(isFailed), ...);
		}

		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr bool IsRequiredAnyGroupsFailed()
		{
			bool isFailed = false;
			(CheckRequiredAnyGroup<typename GetComponentType<tuple_element_t<Indexes, T>>::wrapped>(isFailed, make_index_sequence<tuple_size_v<typename GetComponentType<tuple_element_t<Indexes, T>>::wrapped>>()), ...);
			return isFailed;
		}

		template <typename T, uiw Group, uiw... Indexes> [[nodiscard]] static constexpr auto RequiredAnyGroupToTuple(index_sequence<Indexes...>)
		{
			return make_tuple(pair<TypeId, ui32>(tuple_element_t<Indexes, GetComponentType<T>::expanded>::GetTypeId(), (ui32)Group)...);
		}

		// TODO: more checks
		template <typename T, uiw... Indexes> [[nodiscard]] static constexpr auto RequiredAnyToComponentsArray(index_sequence<Indexes...>)
		{
			if constexpr (IsRequiredAnyGroupsFailed<T, Indexes...>())
			{
				return array<pair<TypeId, ui32>, 0>{};
			}
			else
			{
				constexpr auto converted = Funcs::TupleToArray(tuple_cat(RequiredAnyGroupToTuple<tuple_element_t<Indexes, T>, Indexes>(make_index_sequence<GetComponentType<tuple_element_t<Indexes, T>>::argumentCount>())...));
				if constexpr (converted.empty())
				{
					return array<pair<TypeId, ui32>, 0>{};
				}
				else
				{
					return converted;
				}
			}
		}

		template <uiw NonAnySize, uiw AnySize> [[nodiscard]] static constexpr array<ArchetypeDefiningRequirement, NonAnySize + AnySize > ToArchetypeDefiningRequirement(const array<System::ComponentRequest, NonAnySize> &nonAnyComponents, const array<pair<TypeId, ui32>, AnySize> &anyComponents)
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

		template <typename AcceptType> [[nodiscard]] static constexpr auto AcquireRequestedComponents()
		{
			using rfc = RequirementForComponent;

			using typesFull = typename FunctionInfo::Info<AcceptType>::args;

			constexpr auto environmentIndex = LocateEnvironmentArugument<typesFull>(make_index_sequence<tuple_size_v<typesFull>>());
			constexpr auto entityIDIndex = LocateEntityIDArugument<typesFull>(make_index_sequence<tuple_size_v<typesFull>>());
			using typesWithoutEnvironment = conditional_t<environmentIndex.second != nullopt, Funcs::RemoveTupleElement<environmentIndex.second.value_or(0), typesFull>, typesFull>;

			constexpr auto entityIDIndexToRemove = LocateEntityIDArugument<typesWithoutEnvironment>(make_index_sequence<tuple_size_v<typesWithoutEnvironment>>());
			using typesWithoutEntityID = conditional_t<entityIDIndexToRemove.second != nullopt, Funcs::RemoveTupleElement<entityIDIndexToRemove.second.value_or(0), typesWithoutEnvironment>, typesWithoutEnvironment>;

			using transformedTypes = decltype(TransformArguments<typesWithoutEntityID>(make_index_sequence<tuple_size_v<typesWithoutEntityID>>()));
			using withoutAny = typename transformedTypes::withoutAny;
			using onlyAny = typename transformedTypes::onlyAny;
			using withoutAnyUnpacked = typename decltype(UnpackArguments<withoutAny>(make_index_sequence<tuple_size_v<withoutAny>>()))::unpacked;

			constexpr auto componentsArray = TupleToComponentsArray<withoutAnyUnpacked>(make_index_sequence<tuple_size_v<withoutAnyUnpacked>>());
			constexpr auto sorted = Funcs::SortCompileTime(componentsArray);
			constexpr auto requiredWithoutData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::Required))>(sorted, make_array(rfc::Required));
			constexpr auto requiredWithData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData))>(sorted, make_array(rfc::RequiredWithData));
			constexpr auto required = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData, rfc::Required))>(sorted, make_array(rfc::RequiredWithData, rfc::Required));
			constexpr auto requiredOrOptional = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::OptionalWithData, rfc::Optional))>(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::OptionalWithData, rfc::Optional));
			constexpr auto withData = FindComponentsWithData<FindComponentsWithDataCount<false>(sorted), false>(sorted);
			constexpr auto optionalNoData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::Optional))>(sorted, make_array(rfc::Optional));
			constexpr auto optionalWithData = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::OptionalWithData))>(sorted, make_array(rfc::OptionalWithData));
			constexpr auto subtractive = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::Subtractive))>(sorted, make_array(rfc::Subtractive));
			constexpr auto writeAccess = FindComponentsWithData<FindComponentsWithDataCount<true>(sorted), true>(sorted);
			constexpr auto argumentPassingOrder = FindMatchingComponents<FindMatchingComponentsCount(componentsArray, make_array(rfc::RequiredWithData, rfc::OptionalWithData))>(componentsArray, make_array(rfc::RequiredWithData, rfc::OptionalWithData));
			
			constexpr auto archetypeDefining = FindMatchingComponents<FindMatchingComponentsCount(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::Subtractive))>(sorted, make_array(rfc::RequiredWithData, rfc::Required, rfc::Subtractive));
			constexpr auto requiredAnyArguments = RequiredAnyToComponentsArray<onlyAny>(make_index_sequence<tuple_size_v<onlyAny>>());
			constexpr auto archetypeDefiningInfoOnly = ToArchetypeDefiningRequirement(archetypeDefining, requiredAnyArguments);

			return tuple
			{
				requiredWithoutData,
				requiredWithData,
				required,
				requiredOrOptional,
				withData,
				optionalNoData,
				optionalWithData,
				subtractive,
				writeAccess,
				sorted,
				argumentPassingOrder,
				entityIDIndex.first,
				environmentIndex.first,
				archetypeDefiningInfoOnly
			};
		}

		template <typename Tuple> [[nodiscard]] static constexpr System::Requests ComponentsTupleToRequests(const Tuple &requestedComponentsTuple)
		{
			return 
			{
				ToArray(get<0>(requestedComponentsTuple)), // requiredWithoutData
				ToArray(get<1>(requestedComponentsTuple)), // requiredWithData
				ToArray(get<2>(requestedComponentsTuple)), // required
				ToArray(get<3>(requestedComponentsTuple)), // requiredOrOptional
				ToArray(get<4>(requestedComponentsTuple)), // withData
				ToArray(get<5>(requestedComponentsTuple)), // optionalNoData
				ToArray(get<6>(requestedComponentsTuple)), // optionalWithData
				ToArray(get<7>(requestedComponentsTuple)), // subtractive
				ToArray(get<8>(requestedComponentsTuple)), // writeAccess
				ToArray(get<9>(requestedComponentsTuple)), // sorted
				ToArray(get<10>(requestedComponentsTuple)), // argumentPassingOrder
				get<11>(requestedComponentsTuple), // entityIDIndex
				get<12>(requestedComponentsTuple), // environmentIndex
				ToArray(get<13>(requestedComponentsTuple)) // archetypeDefiningInfoOnly
			};
		}
    };

	template <typename SystemType> struct IndirectSystem : public BaseIndirectSystem, public TypeIdentifiable<SystemType>
	{
		[[nodiscard]] static constexpr auto AcquireRequestedComponents()
		{
			return _SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>();
		}

	public:
		[[nodiscard]] virtual TypeId GetTypeId() const override
		{
			return TypeIdentifiable<SystemType>::GetTypeId();
		}
		
		[[nodiscard]] virtual string_view GetTypeName() const override
		{
			return TypeIdentifiable<SystemType>::GetTypeName();
		}
			
		[[nodiscard]] virtual const Requests &RequestedComponents() const override final
		{
			static constexpr auto requestedComponentsTuple = AcquireRequestedComponents();
			static constexpr Requests requestedComponentsArray = _SystemHelperFuncs::ComponentsTupleToRequests(requestedComponentsTuple);
			static_assert(requestedComponentsArray.entityIDIndex == nullopt, "Indirect systems cannot request EntityID");
			static_assert(requestedComponentsArray.environmentIndex == nullopt, "Indirect systems cannot request Environment");
			return requestedComponentsArray;
		}
	};

	template <typename SystemType> struct DirectSystem : public BaseDirectSystem, public TypeIdentifiable<SystemType>
	{
		[[nodiscard]] static constexpr auto AcquireRequestedComponents()
		{
			return _SystemHelperFuncs::AcquireRequestedComponents<decltype(&SystemType::Accept)>();
		}

	public:
		[[nodiscard]] virtual TypeId GetTypeId() const override final
		{
			return TypeIdentifiable<SystemType>::GetTypeId();
		}

		[[nodiscard]] virtual string_view GetTypeName() const override final
		{
			return TypeIdentifiable<SystemType>::GetTypeName();
		}

		[[nodiscard]] virtual const Requests &RequestedComponents() const override final
		{
			static constexpr auto requestedComponentsTuple = AcquireRequestedComponents();
			static constexpr Requests requestedComponentsArray = _SystemHelperFuncs::ComponentsTupleToRequests(requestedComponentsTuple);
			return requestedComponentsArray;
		}

		virtual void AcceptUntyped(void **array) override final
		{
			using types = typename FunctionInfo::Info<decltype(&SystemType::Accept)>::args;
			static constexpr uiw count = tuple_size_v<types>;
			_SystemHelperFuncs::CallAccept<types>((SystemType *)this, array, make_index_sequence<count>());
		}
	};
}