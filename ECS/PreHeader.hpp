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

using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::string_view;
using std::pair;
using std::function;
using std::array;
using std::move;
using std::swap;
using std::make_pair;
using std::make_shared;
using std::make_unique;
using namespace std::literals;

#include <StdMiscellaneousLib.hpp>
#include <MatrixMathTypes.hpp>
using namespace StdLib;

#include "FunctionInfo.hpp"
#include "Array.hpp"

struct AlignedMallocDeleter
{
    void operator()(ui8 *ptr)
    {
        _aligned_free(ptr);
    }
};