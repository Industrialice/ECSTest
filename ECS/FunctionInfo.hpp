#pragma once

#include <functional>

// TODO: refactor it and move into StdLib2018

namespace StdLib
{
    template <typename T>
    struct FunctionInfo {};

    template <typename T>
    struct FunctionInfo< T * > {};

    template <typename T, typename Class>
    struct FunctionInfo< T(Class::*) > {};


    template <typename Result, typename ...Args>
    struct FunctionInfo< Result(Args...) >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(*) (Args...);
        using clazz = void;
    };

    template <typename Result, typename ...Args>
    struct FunctionInfo< Result(*) (Args...) >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(*) (Args...);
        using clazz = void;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...);
        using clazz = Class;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) const >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) const;
        using clazz = Class;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) volatile >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) volatile;
        using clazz = Class;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) volatile const >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) volatile const;
        using clazz = Class;
    };

    template <typename Result, typename ...Args>
    struct FunctionInfo< Result(Args...) noexcept >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(*) (Args...) noexcept;
        using clazz = void;
    };

    template <typename Result, typename ...Args>
    struct FunctionInfo< Result(*) (Args...) noexcept >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(*) (Args...) noexcept;
        using clazz = void;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) noexcept >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) noexcept;
        using clazz = Class;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) const noexcept >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) const noexcept;
        using clazz = Class;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) volatile noexcept >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) volatile noexcept;
        using clazz = Class;
    };

    template <typename Class, typename Result, typename ...Args>
    struct FunctionInfo< Result(Class::*) (Args...) volatile const noexcept >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(Class::*) (Args...) volatile const noexcept;
        using clazz = Class;
    };

    template <typename Result, typename ...Args>
    struct FunctionInfo< std::function< Result(Args...) > >
    {
        using args = std::tuple<Args...>;
        using result = Result;
        using type = Result(*) (Args...);
        using clazz = void;
    };
}