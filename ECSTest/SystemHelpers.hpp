#pragma once

namespace ECSTest
{
    template <auto Method, typename types = typename FunctionInfo<decltype(Method)>::args, uiw count = std::tuple_size_v<types>> struct _AcceptCaller
    {
        template <typename T> static FORCEINLINE auto Convert(void *arg) -> decltype(auto)
        {
            static_assert(std::is_reference_v<T> || std::is_pointer_v<T>, "Type must be either reference or pointer");

            if constexpr (std::is_reference_v<T>)
            {
                using bare = std::remove_reference_t<T>;
                static_assert(!std::is_pointer_v<bare>, "Type cannot be reference to pointer");
                return *(bare *)arg;
            }
            else
            {
                using bare = std::remove_pointer_t<T>;
                static_assert(!std::is_pointer_v<bare> && !std::is_reference_v<bare>, "Type cannot be pointer to reference/pointer");
                return (T)arg;
            }
        }

        template <typename T, typename = std::enable_if_t<count == 1>> static FORCEINLINE void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            (object->*Method)(Convert<t0>(first));
        }

        template <typename T, typename = std::enable_if_t<count == 2>, typename = std::enable_if_t<count == 2>> static FORCEINLINE void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            decltype(auto) a1 = Convert<t1>(va_arg(args, void *));
            (object->*Method)(Convert<t0>(first), a1);
        }

        template <typename T, typename = std::enable_if_t<count == 3>, typename = std::enable_if_t<count == 3>, typename = std::enable_if_t<count == 3>> static FORCEINLINE void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            using t2 = std::tuple_element_t<2, types>;
            decltype(auto) a1 = Convert<t1>(va_arg(args, void *));
            decltype(auto) a2 = Convert<t2>(va_arg(args, void *));
            (object->*Method)(Convert<t0>(first), a1, a2);
        }

        template <typename T, typename = std::enable_if_t<count == 4>, typename = std::enable_if_t<count == 4>, typename = std::enable_if_t<count == 4>, typename = std::enable_if_t<count == 4>> static FORCEINLINE void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            using t2 = std::tuple_element_t<2, types>;
            using t3 = std::tuple_element_t<3, types>;
            decltype(auto) a1 = Convert<t1>(va_arg(args, void *));
            decltype(auto) a2 = Convert<t2>(va_arg(args, void *));
            decltype(auto) a3 = Convert<t3>(va_arg(args, void *));
            (object->*Method)(Convert<t0>(first), a1, a2, a3);
        }

        template <typename T, typename = std::enable_if_t<count == 5>, typename = std::enable_if_t<count == 5>, typename = std::enable_if_t<count == 5>, typename = std::enable_if_t<count == 5>, typename = std::enable_if_t<count == 5>> static FORCEINLINE void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            using t2 = std::tuple_element_t<2, types>;
            using t3 = std::tuple_element_t<3, types>;
            using t4 = std::tuple_element_t<4, types>;
            decltype(auto) a1 = Convert<t1>(va_arg(args, void *));
            decltype(auto) a2 = Convert<t2>(va_arg(args, void *));
            decltype(auto) a3 = Convert<t3>(va_arg(args, void *));
            decltype(auto) a4 = Convert<t4>(va_arg(args, void *));
            (object->*Method)(Convert<t0>(first), a1, a2, a3, a4);
        }

        template <typename T, typename = std::enable_if_t<count == 6>, typename = std::enable_if_t<count == 6>, typename = std::enable_if_t<count == 6>, typename = std::enable_if_t<count == 6>, typename = std::enable_if_t<count == 6>, typename = std::enable_if_t<count == 6>> static FORCEINLINE void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            using t2 = std::tuple_element_t<2, types>;
            using t3 = std::tuple_element_t<3, types>;
            using t4 = std::tuple_element_t<4, types>;
            using t5 = std::tuple_element_t<5, types>;
            decltype(auto) a1 = Convert<t1>(va_arg(args, void *));
            decltype(auto) a2 = Convert<t2>(va_arg(args, void *));
            decltype(auto) a3 = Convert<t3>(va_arg(args, void *));
            decltype(auto) a4 = Convert<t4>(va_arg(args, void *));
            decltype(auto) a5 = Convert<t5>(va_arg(args, void *));
            (object->*Method)(Convert<t0>(first), a1, a2, a3, a4, a5);
        }
    };

    template <typename T> constexpr System::RequestedComponent _ArgumentToComponent()
    {
        using TPure = std::remove_pointer_t<std::remove_reference_t<T>>;
        return {TPure::GetTypeId(), !std::is_const_v<TPure>, std::is_reference_v<T>};
    }

    template <auto Method, typename T = typename FunctionInfo<decltype(Method)>::args, uiw size = std::tuple_size_v<T>> struct _TupleToComponents;

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

#define ACCEPT_COMPONENTS(...) \
    virtual pair<const RequestedComponent *, uiw> RequestedComponents() const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
        auto arr = _TupleToComponents<&thisType::Accept>::Convert(); \
        return {arr.data(), arr.size()}; \
    } \
    \
    virtual void AcceptComponents(void *first, ...) const override \
    { \
        using thisType = std::remove_reference_t<std::remove_cv_t<decltype(*this)>>; \
        va_list args; \
        va_start(args, first); \
        _AcceptCaller<&thisType::Accept>::Call(this, first, args); \
        va_end(args); \
    } \
    void Accept(__VA_ARGS__) const