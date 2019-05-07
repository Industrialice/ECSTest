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