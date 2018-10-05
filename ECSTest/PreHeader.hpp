#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::string_view;
using std::pair;
using std::function;
using std::move;
using std::swap;
using std::make_pair;
using std::make_shared;
using namespace std::literals;

#include <StdPlatformAbstractionLib.hpp>
#include <MatrixMathTypes.hpp>
using namespace StdLib;

static constexpr uiw MaxSystemDataPointers = 8;