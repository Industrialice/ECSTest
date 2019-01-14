#pragma once

namespace ECSTest
{
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

    template <auto Method, typename types = typename StdLib::FunctionInfo<decltype(Method)>::args, uiw count = std::tuple_size_v<types>> struct _AcceptCaller
    {
        template <typename T> static FORCEINLINE auto Convert(void *arg) -> decltype(auto)
        {
			using pureType = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;

			static constexpr bool isRefOrPtr = std::is_reference_v<T> || std::is_pointer_v<T>;
			static constexpr bool isSubtractive = std::is_base_of_v<_SubtractiveComponentBase, pureType>;
			static constexpr bool isArray = std::is_base_of_v<_ArrayBase, pureType>;
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

		template <typename T> static FORCEINLINE void Call(T *object, void **array, std::integral_constant<uiw, 1>)
		{
			using t0 = std::tuple_element_t<0, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			(object->*Method)(a0);
		}

		template <typename T> static FORCEINLINE void Call(T *object, void **array, std::integral_constant<uiw, 2>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			(object->*Method)(a0, a1);
		}

		template <typename T> static FORCEINLINE void Call(T *object, void **array, std::integral_constant<uiw, 3>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			(object->*Method)(a0, a1, a2);
		}

		template <typename T> static FORCEINLINE void Call(T *object, void **array, std::integral_constant<uiw, 4>)
		{
			using t0 = std::tuple_element_t<0, types>;
			using t1 = std::tuple_element_t<1, types>;
			using t2 = std::tuple_element_t<2, types>;
			using t3 = std::tuple_element_t<3, types>;
			decltype(auto) a0 = Convert<t0>(array[0]);
			decltype(auto) a1 = Convert<t1>(array[1]);
			decltype(auto) a2 = Convert<t2>(array[2]);
			decltype(auto) a3 = Convert<t3>(array[3]);
			(object->*Method)(a0, a1, a2, a3);
		}

		template <typename T> static FORCEINLINE void Call(T *object, void **array, std::integral_constant<uiw, 5>)
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
			(object->*Method)(a0, a1, a2, a3, a4);
		}

		template <typename T> static FORCEINLINE void Call(T *object, void **array, std::integral_constant<uiw, 6>)
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
			(object->*Method)(a0, a1, a2, a3, a4, a5);
		}
    };

    template <typename T> constexpr System::RequestedComponent _ArgumentToComponent()
    {
        using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
		using componentType = typename _GetComponentType<std::remove_cv_t<TPure>>::type;
		System::ComponentAvailability availability;
		if constexpr (std::is_reference_v<T>)
		{
			availability = System::ComponentAvailability::Required;
		}
		else if constexpr (std::is_pointer_v<T>)
		{
			availability = System::ComponentAvailability::Optional;
		}
		else if constexpr (std::is_base_of_v<_SubtractiveComponentBase, T>)
		{
			availability = System::ComponentAvailability::Subtractive;
		}
		return {componentType::GetTypeId(), !std::is_const_v<TPure>, availability};
    }

    template <auto Method, typename T = typename StdLib::FunctionInfo<decltype(Method)>::args, uiw size = std::tuple_size_v<T>> struct _TupleToComponents;

    template <auto Method, typename T> struct _TupleToComponents<Method, T, 1>
    {
        static constexpr std::array<System::RequestedComponent, 1> Convert()
        {
            return
            {
                _ArgumentToComponent<std::tuple_element_t<0, T>>(),
            };
        }
    };

    template <auto Method, typename T> struct _TupleToComponents<Method, T, 2>
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

    template <auto Method, typename T> struct _TupleToComponents<Method, T, 3>
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

    template <auto Method, typename T> struct _TupleToComponents<Method, T, 4>
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

    template <auto Method, typename T> struct _TupleToComponents<Method, T, 5>
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

    template <auto Method, typename T> struct _TupleToComponents<Method, T, 6>
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
    virtual pair<const RequestedComponent *, uiw> RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
        auto arr = _TupleToComponents<&thisType::Accept>::Convert(); \
        return {arr.data(), arr.size()}; \
    } \
    \
    virtual void Accept(void **array) override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		using types = typename FunctionInfo<decltype(&thisType::Accept)>::args; \
		static constexpr uiw count = std::tuple_size_v<types>; \
        _AcceptCaller<&thisType::Accept, types>::Call(this, array, std::integral_constant<uiw, count>()); \
    } \
    void Accept(__VA_ARGS__)

#define INDIRECT_ACCEPT_COMPONENTS(...) \
    virtual pair<const RequestedComponent *, uiw> RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
        auto arr = _TupleToComponents<&thisType::Accept>::Convert(); \
        return {arr.data(), arr.size()}; \
    } \
    \
    virtual void Accept(void **array) override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
		using types = typename FunctionInfo<decltype(&thisType::Accept)>::args; \
		static constexpr uiw count = std::tuple_size_v<types>; \
        _AcceptCaller<&thisType::Accept, types>::Call(this, array, std::integral_constant<uiw, count>()); \
    } \
    virtual void Remove(Entity *entity) override; \
    virtual void Commit() override; \
    virtual void Clear() override; \
    virtual void OnChanged(const shared_ptr<vector<Component>> &components) override; \
    virtual void Update() override; \
    void Acceptor(__VA_ARGS__)