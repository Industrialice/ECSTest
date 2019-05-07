#pragma once

#include <PreHeader.hpp>
#include <forward_list>
#include <SystemsManager.hpp>
#include <KeyController.hpp>
#include "WinAPI.hpp"

using namespace ECSTest;

inline shared_ptr<Logger<string_view, true>> EngineLogger{};

#ifdef DEBUG
    // this snprintf will trigger compiler warnings if incorrect args were passed in
    #define SENDLOG(level, sender, ...) snprintf(nullptr, 0, __VA_ARGS__); EngineLogger->Message(StdLib::LogLevels:: level, TOSTR(sender), __VA_ARGS__)
#else
    #define SENDLOG(level, sender, ...) EngineLogger->Message(StdLib::LogLevels:: level, TOSTR(sender), __VA_ARGS__)
#endif