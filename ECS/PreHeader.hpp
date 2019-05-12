#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <array>
#include <unordered_map>
#include <map>
#include <cstdlib>
#include <stack>
#include <experimental/coroutine>
#include <experimental/generator>

#include <StdMiscellaneousLib.hpp>
#include <MatrixMathTypes.hpp>
#include <Logger.hpp>
using namespace StdLib;

using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::string_view;
using std::pair;
using std::function;
using std::array;
using std::optional;
using std::variant;
using std::to_string;
using std::move;
using std::swap;
using std::make_pair;
using std::make_shared;
using std::make_unique;
using std::nullopt;
using std::enable_if_t;
using std::conditional_t;
using std::is_base_of_v;
using std::is_same_v;
using namespace std::literals;
using namespace std::placeholders;

//namespace std
//{
//    using experimental::generator;
//}

#include "Array.hpp"

struct MallocDeleter
{
    void operator()(void *ptr)
    {
        free(ptr);
    }
};

struct AlignedMallocDeleter
{
    void operator()(void *ptr)
    {
        _aligned_free(ptr);
    }
};

class UnitTests;

// temporary emulation of std::make_array, taken from https://en.cppreference.com/w/cpp/experimental/make_array
namespace _details 
{
	template<class> struct is_ref_wrapper : std::false_type {};
	template<class T> struct is_ref_wrapper<std::reference_wrapper<T>> : std::true_type {};

	template<class T>
	using not_ref_wrapper = std::negation<is_ref_wrapper<std::decay_t<T>>>;

	template <class D, class...> struct return_type_helper { using type = D; };
	template <class... Types>
	struct return_type_helper<void, Types...> : std::common_type<Types...> 
	{
		static_assert(std::conjunction_v<not_ref_wrapper<Types>...>,
			"Types cannot contain reference_wrappers when D is void");
	};

	template <class D, class... Types>
	using return_type = std::array<typename return_type_helper<D, Types...>::type,
		sizeof...(Types)>;
}

template <class D = void, class... Types> constexpr _details::return_type<D, Types...> make_array(Types&&... t) 
{
	return {std::forward<Types>(t)...};
}