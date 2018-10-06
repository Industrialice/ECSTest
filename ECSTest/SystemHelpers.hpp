#pragma once

namespace ECSTest
{
    template <auto Method, typename types = typename FunctionInfo<decltype(Method)>::args, uiw count = std::tuple_size_v<types>> struct _AcceptCaller
    {
        template <typename T, typename = std::enable_if_t<count == 1>> static void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            (object->*Method)((t0)first);
        }

        template <typename T, typename = std::enable_if_t<count == 2>, typename = std::enable_if_t<count == 2>> static void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            (object->*Method)((t0)first, (t1)va_arg(args, void *));
        }

        template <typename T, typename = std::enable_if_t<count == 3>, typename = std::enable_if_t<count == 3>, typename = std::enable_if_t<count == 3>> static void Call(T *object, void *first, va_list args)
        {
            using t0 = std::tuple_element_t<0, types>;
            using t1 = std::tuple_element_t<1, types>;
            using t2 = std::tuple_element_t<2, types>;
            (object->*Method)((t0)first, (t1)va_arg(args, void *), (t2)va_arg(args, void *));
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
                _ArgumentToComponent<std::tuple_element_t<0, T>>()
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
                _ArgumentToComponent<std::tuple_element_t<1, T>>()
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
                _ArgumentToComponent<std::tuple_element_t<2, T>>()
            };
        }
    };
}

#define ACCEPT_SLIM_COMPONENTS(...) \
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