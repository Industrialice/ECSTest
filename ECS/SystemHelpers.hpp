#pragma once

namespace ECSTest
{
    constexpr bool operator < (const System::RequestedComponent &left, const System::RequestedComponent &right)
    {
        return std::tie(left.isWriteAccess, left.requirement, left.type) < std::tie(right.isWriteAccess, right.requirement, right.type);
    }

	template <typename T> struct _GetComponentType
	{
		using type = T;
	};

	template <typename T> struct _GetComponentType<SubtractiveComponent<T>>
	{
		using type = T;
	};

	template <typename T> struct _GetComponentType<Array<T>>
	{
		using type = T;
	};

	template <typename T> struct _GetComponentType<Array<SubtractiveComponent<T>>>
	{
		//static_assert(false, "Subtractive component cannot be inside an array");
		using type = T;
	};

	template <typename T> constexpr bool _IsArray(Array<T>) { return true; }
	template <typename T> constexpr bool _IsArray(T) { return false; }

    // used by direct systems to convert void **array into proper function argument types
    template <auto Method, typename types = typename FunctionInfo::Info<decltype(Method)>::args, uiw count = std::tuple_size_v<types>> struct _AcceptCaller
    {
        // converts void * into a properly typed T argument - a pointer or a reference
        template <typename T> static FORCEINLINE auto Convert(void *arg) -> decltype(auto)
        {
			using pureType = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;

			static constexpr bool isRefOrPtr = std::is_reference_v<T> || std::is_pointer_v<T>;
			static constexpr bool isSubtractive = std::is_base_of_v<_SubtractiveComponentBase, pureType>;
			static constexpr bool isArray = _IsArray(pureType());
            static_assert(isSubtractive || isRefOrPtr, "Type must be either reference or pointer");
			static_assert(!isSubtractive || !isRefOrPtr, "Subtractive component cannot be passed by reference or pointer");
			static_assert(isSubtractive || isArray, "Non-subtractive components must be passed using Array<T>");
			static_assert(!isSubtractive || !isArray, "Subtractive component cannot be inside an array");

			using componentType = typename _GetComponentType<pureType>::type;

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

		template <typename T> static FORCEINLINE void Call(T *object, System::Environment &env, void **array, std::integral_constant<uiw, 1>)
		{
			using t0 = std::tuple_element_t<0, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			(object->*Method)(env, a0);
		}

		template <typename T> static FORCEINLINE void Call(T *object, System::Environment &env, void **array, std::integral_constant<uiw, 2>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			(object->*Method)(env, a0, a1);
		}

		template <typename T> static FORCEINLINE void Call(T *object, System::Environment &env, void **array, std::integral_constant<uiw, 3>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			(object->*Method)(env, a0, a1, a2);
		}

		template <typename T> static FORCEINLINE void Call(T *object, System::Environment &env, void **array, std::integral_constant<uiw, 4>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			using t3 = std::tuple_element_t<3, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			decltype(auto) a3 = Convert<t3>(array[3]);
			(object->*Method)(env, a0, a1, a2, a3);
		}

		template <typename T> static FORCEINLINE void Call(T *object, System::Environment &env, void **array, std::integral_constant<uiw, 5>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			using t3 = std::tuple_element_t<3, types>;
			using t4 = std::tuple_element_t<4, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			decltype(auto) a3 = Convert<t3>(array[3]);
			decltype(auto) a4 = Convert<t4>(array[4]);
			(object->*Method)(env, a0, a1, a2, a3, a4);
		}

		template <typename T> static FORCEINLINE void Call(T *object, System::Environment &env, void **array, std::integral_constant<uiw, 6>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			using t3 = std::tuple_element_t<3, types>;
			using t4 = std::tuple_element_t<4, types>;
			using t5 = std::tuple_element_t<5, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			decltype(auto) a3 = Convert<t3>(array[3]);
			decltype(auto) a4 = Convert<t4>(array[4]);
			decltype(auto) a5 = Convert<t5>(array[5]);
			(object->*Method)(env, a0, a1, a2, a3, a4, a5);
		}
    };

    // converts argument type (like Array<Component> &) into System::RequestedComponent
    template <typename T> constexpr System::RequestedComponent _ArgumentToComponent()
    {
        using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
		using componentType = typename _GetComponentType<std::remove_cv_t<TPure>>::type;
		constexpr RequirementForComponent availability =
			std::is_reference_v<T> ?
			RequirementForComponent::Required :
			(std::is_pointer_v<T> ?
				RequirementForComponent::Optional :
				(std::is_base_of_v<_SubtractiveComponentBase, T> ?
					RequirementForComponent::Subtractive :
					(RequirementForComponent)i8_max));
		static_assert((i32)availability != i8_max);
		return {componentType::GetTypeId(), !std::is_const_v<TPure>, availability};
    }

    template <typename T, uiw... Indexes> constexpr std::array<System::RequestedComponent, sizeof...(Indexes)> _TupleToComponentsArray(std::index_sequence<Indexes...>)
    {
        return {_ArgumentToComponent<std::tuple_element_t<Indexes, T>>()...};
    }

    template <uiw size> [[nodiscard]] constexpr pair<std::array<System::RequestedComponent, size>, uiw> _FindMatchingComponents(const std::array<System::RequestedComponent, size> &arr, RequirementForComponent requirement)
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

    template <uiw size> [[nodiscard]] constexpr pair<std::array<System::RequestedComponent, size>, uiw> _FindComponentsWithWriteAccess(const std::array<System::RequestedComponent, size> &arr)
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

    template <uiw size> [[nodiscard]] constexpr pair<std::array<pair<StableTypeId, RequirementForComponent>, size>, uiw> _FindArchetypeDefiningComponents(const std::array<System::RequestedComponent, size> &arr)
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
}

// TODO: optimize it so std::arrays use only as much space as they actually need

#define DIRECT_ACCEPT_COMPONENTS(...) \
    virtual const Requests &RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		struct Local { static void Temp(__VA_ARGS__); }; \
		using types = typename FunctionInfo::Info<decltype(Local::Temp)>::args; \
        static constexpr auto arr = _TupleToComponentsArray<types>(std::make_index_sequence<std::tuple_size_v<types>>()); \
        static constexpr auto arrSorted = Funcs::SortCompileTime(arr); \
        static constexpr auto required = _FindMatchingComponents(arrSorted, RequirementForComponent::Required); \
        static constexpr auto opt = _FindMatchingComponents(arrSorted, RequirementForComponent::Optional); \
        static constexpr auto subtractive = _FindMatchingComponents(arrSorted, RequirementForComponent::Subtractive); \
        static constexpr auto writeAccess = _FindComponentsWithWriteAccess(arrSorted); \
        static constexpr auto archetypeDefining = _FindArchetypeDefiningComponents(arrSorted); \
        static constexpr Requests requests = \
        { \
            ToArray(required.first.data(), required.second), \
            ToArray(opt.first.data(), opt.second), \
            ToArray(subtractive.first.data(), subtractive.second), \
            ToArray(writeAccess.first.data(), writeAccess.second), \
            ToArray(archetypeDefining.first.data(), archetypeDefining.second), \
            ToArray(arrSorted), \
            ToArray(arr) \
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
        _AcceptCaller<&thisType::Update, types>::Call(this, env, array, std::integral_constant<uiw, count>()); \
    } \
    void Update(Environment &env, __VA_ARGS__)

#define INDIRECT_ACCEPT_COMPONENTS(...) \
    virtual const Requests &RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		struct Local { static void Temp(__VA_ARGS__); }; \
		using types = typename FunctionInfo::Info<decltype(Local::Temp)>::args; \
        static constexpr auto arr = _TupleToComponentsArray<types>(std::make_index_sequence<std::tuple_size_v<types>>()); \
        static constexpr auto arrSorted = Funcs::SortCompileTime(arr); \
        static constexpr auto required = _FindMatchingComponents(arrSorted, RequirementForComponent::Required); \
        static constexpr auto opt = _FindMatchingComponents(arrSorted, RequirementForComponent::Optional); \
        static constexpr auto subtractive = _FindMatchingComponents(arrSorted, RequirementForComponent::Subtractive); \
        static constexpr auto writeAccess = _FindComponentsWithWriteAccess(arrSorted); \
        static constexpr auto archetypeDefining = _FindArchetypeDefiningComponents(arrSorted); \
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