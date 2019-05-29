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
#include <compare>
#include <experimental/coroutine>
#include <experimental/generator>

#include <StdMiscLib.hpp>
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
using std::tuple;
using std::index_sequence;
using std::to_string;
using std::move;
using std::tuple_cat;
using std::swap;
using std::get;
using std::get_if;
using std::make_index_sequence;
using std::tuple_size_v;
using std::is_empty_v;
using std::is_pointer_v;
using std::is_reference_v;
using std::is_const_v;
using std::remove_cv_t;
using std::remove_pointer_t;
using std::remove_reference_t;
using std::declval;
using std::make_pair;
using std::make_shared;
using std::make_unique;
using std::make_array;
using std::nullopt;
using std::tuple_element_t;
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
		Allocator::Malloc::Free(ptr);
    }
};

struct AlignedMallocDeleter
{
    void operator()(void *ptr)
    {
        Allocator::MallocAlignedRuntime::Free(ptr);
    }
};

class UnitTests;