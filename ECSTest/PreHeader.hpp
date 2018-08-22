#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>

using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::string_view;
using std::pair;
using std::move;
using std::swap;
using std::make_pair;
using namespace std::literals;

#include <StdPlatformAbstractionLib.hpp>
#include <MatrixMathTypes.hpp>
using namespace StdLib;

static constexpr uiw MaxSystemDataPointers = 8;