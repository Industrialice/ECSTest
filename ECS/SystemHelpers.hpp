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

    template <auto Method, typename types = typename FunctionInfo::Info<decltype(Method)>::args, uiw count = std::tuple_size_v<types>> struct _AcceptCaller
    {
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
				return T();
			}
        }

		template <typename T> static FORCEINLINE void Call(T *object, const System::Environment &env, void **array, std::integral_constant<uiw, 1>)
		{
			using t0 = std::tuple_element_t<0, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			(object->*Method)(env, a0);
		}

		template <typename T> static FORCEINLINE void Call(T *object, const System::Environment &env, void **array, std::integral_constant<uiw, 2>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			(object->*Method)(env, a0, a1);
		}

		template <typename T> static FORCEINLINE void Call(T *object, const System::Environment &env, void **array, std::integral_constant<uiw, 3>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			(object->*Method)(env, a0, a1, a2);
		}

		template <typename T> static FORCEINLINE void Call(T *object, const System::Environment &env, void **array, std::integral_constant<uiw, 4>)
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

		template <typename T> static FORCEINLINE void Call(T *object, const System::Environment &env, void **array, std::integral_constant<uiw, 5>)
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

		template <typename T> static FORCEINLINE void Call(T *object, const System::Environment &env, void **array, std::integral_constant<uiw, 6>)
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

    template <typename T> constexpr System::RequestedComponent _ArgumentToComponent()
    {
        using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
		using componentType = typename _GetComponentType<std::remove_cv_t<TPure>>::type;
		constexpr System::RequirementForComponent availability =
			std::is_reference_v<T> ?
			System::RequirementForComponent::Required :
			(std::is_pointer_v<T> ?
				System::RequirementForComponent::Optional :
				(std::is_base_of_v<_SubtractiveComponentBase, T> ?
					RequirementForComponent::Subtractive :
					(RequirementForComponent)i8_max));
		static_assert((i32)availability != i8_max);
		return {componentType::GetTypeId(), !std::is_const_v<TPure>, availability};
    }

    template <typename T, uiw size = std::tuple_size_v<T>> struct _TupleToComponents;

	template <typename T> struct _TupleToComponents<T, 0>
	{
		static constexpr std::array<System::RequestedComponent, 1> Convert()
		{
			return
			{
				System::RequestedComponent{{}, false, RequirementForComponent::Optional} // kind of hacky
			};
		}
	};

    template <typename T> struct _TupleToComponents<T, 1>
    {
        static constexpr std::array<System::RequestedComponent, 1> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
            };
        }
    };

    template <typename T> struct _TupleToComponents<T, 2>
    {
        static constexpr std::array<System::RequestedComponent, 2> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
                _ArgumentToComponent<std::tuple_element_t<1, T>>(),
            };
        }
    };

    template <typename T> struct _TupleToComponents<T, 3>
    {
        static constexpr std::array<System::RequestedComponent, 3> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
                _ArgumentToComponent<std::tuple_element_t<1, T>>(),
                _ArgumentToComponent<std::tuple_element_t<2, T>>(),
            };
        }
    };

    template <typename T> struct _TupleToComponents<T, 4>
    {
        static constexpr std::array<System::RequestedComponent, 4> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
                _ArgumentToComponent<std::tuple_element_t<1, T>>(),
                _ArgumentToComponent<std::tuple_element_t<2, T>>(),
                _ArgumentToComponent<std::tuple_element_t<3, T>>(),
            };
        }
    };

    template <typename T> struct _TupleToComponents<T, 5>
    {
        static constexpr std::array<System::RequestedComponent, 5> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
                _ArgumentToComponent<std::tuple_element_t<1, T>>(),
                _ArgumentToComponent<std::tuple_element_t<2, T>>(),
                _ArgumentToComponent<std::tuple_element_t<3, T>>(),
                _ArgumentToComponent<std::tuple_element_t<4, T>>(),
            };
        }
    };

    template <typename T> struct _TupleToComponents<T, 6>
    {
        static constexpr std::array<System::RequestedComponent, 6> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
                _ArgumentToComponent<std::tuple_element_t<1, T>>(),
                _ArgumentToComponent<std::tuple_element_t<2, T>>(),
                _ArgumentToComponent<std::tuple_element_t<3, T>>(),
                _ArgumentToComponent<std::tuple_element_t<4, T>>(),
                _ArgumentToComponent<std::tuple_element_t<5, T>>(),
            };
        }
    };
}

#define DIRECT_ACCEPT_COMPONENTS(...) \
    virtual Array<const RequestedComponent> RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		using types = typename FunctionInfo::Info<decltype(&thisType::Acceptor)>::args; \
        static constexpr auto arr = _TupleToComponents<types>::Convert(); \
        static constexpr auto arrSorted = Funcs::SortCompileTime(arr); \
        return {arrSorted.data(), arrSorted.size()}; \
    } \
    \
    virtual void Accept(const Environment &env, void **array) override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		using types = typename FunctionInfo::Info<decltype(&thisType::Acceptor)>::args; \
		static constexpr uiw count = std::tuple_size_v<types>; \
        _AcceptCaller<&thisType::Acceptor, types>::Call(this, array, std::integral_constant<uiw, count>()); \
    } \
    void Acceptor(const Environment &env, __VA_ARGS__)

#define INDIRECT_ACCEPT_COMPONENTS(...) \
    virtual Array<const RequestedComponent> RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		struct Local { static void Temp(__VA_ARGS__); }; \
		using types = typename FunctionInfo::Info<decltype(Local::Temp)>::args; \
        static constexpr auto arr = _TupleToComponents<types>::Convert(); \
        static constexpr auto arrSorted = Funcs::SortCompileTime(arr); \
        return {arrSorted.data(), arrSorted.size()}; \
    } \
    \
	virtual void ProcessMessages(const MessageStreamEntityAdded &stream) override; \
	virtual void ProcessMessages(const MessageStreamEntityRemoved &stream) override; \
    virtual void Update(const Environment &env, MessageBuilder &messageBuilder) override