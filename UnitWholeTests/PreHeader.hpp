#pragma once

#include <algorithm>
#include <numeric>
#include <PreHeader.hpp>
#include <SystemsManager.hpp>
#include <SystemCreation.hpp>
#include <Windows.h>
#include <EntitiesStreamBuilder.hpp>
#include <KeyController.hpp>
#include <ArchetypeReflector.hpp>
#include <set>

namespace ECSTest
{
    inline const char *LogLevelToTag(LogLevels::LogLevel logLevel)
    {
        switch (logLevel.AsInteger())
        {
        case LogLevels::Critical.AsInteger():
            return "[crt] ";
        case LogLevels::Debug.AsInteger():
            return "[dbg] ";
        case LogLevels::Error.AsInteger():
            return "[err] ";
        case LogLevels::Attention.AsInteger():
            return "[imp] ";
        case LogLevels::Info.AsInteger():
            return "[inf] ";
        case LogLevels::Other.AsInteger():
            return "[oth] ";
        case LogLevels::Warning.AsInteger():
            return "[wrn] ";
        case LogLevels::_None.AsInteger():
        case LogLevels::_All.AsInteger():
            HARDBREAK;
            return "";
        }

        UNREACHABLE;
        return nullptr;
    }

    inline void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName)
    {
        if (logLevel == LogLevels::Critical || logLevel == LogLevels::Debug || logLevel == LogLevels::Error) // TODO: cancel breaking
        {
            SOFTBREAK;
        }

        if (logLevel == LogLevels::Critical/* || logLevel == LogLevel::Debug || logLevel == LogLevel::Error*/)
        {
            const char *tag = nullptr;
            switch (logLevel.AsInteger()) // fill out all the cases just in case
            {
            case LogLevels::Critical.AsInteger():
                tag = "CRITICAL";
                break;
            case LogLevels::Debug.AsInteger():
                tag = "DEBUG";
                break;
            case LogLevels::Error.AsInteger():
                tag = "ERROR";
                break;
            case LogLevels::Attention.AsInteger():
                tag = "IMPORTANT INFO";
                break;
            case LogLevels::Info.AsInteger():
                tag = "INFO";
                break;
            case LogLevels::Other.AsInteger():
                tag = "OTHER";
                break;
            case LogLevels::Warning.AsInteger():
                tag = "WARNING";
                break;
            case LogLevels::_None.AsInteger():
            case LogLevels::_All.AsInteger():
                HARDBREAK;
                return;
            }

            MessageBoxA(0, nullTerminatedText.data(), tag, 0);
            return;
        }

        const char *tag = LogLevelToTag(logLevel);

        OutputDebugStringA(tag);
        OutputDebugStringA(senderName.data());
        OutputDebugStringA(": ");
        OutputDebugStringA(nullTerminatedText.data());

        printf("%s%s: %s", tag, senderName.data(), nullTerminatedText.data());
    }
}