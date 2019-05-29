#pragma once

#ifdef _MSC_VER
	#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
	#define NOVIRTUALKEYCODES // VK_ *
	#define NOWINMESSAGES     // WM_ *, EM_ *, LB_ *, CB_ *
	#define NOWINSTYLES       // WS_ *, CS_ *, ES_ *, LBS_ *, SBS_ *, CBS_ *
//	#define NOSYSMETRICS      // SM_ *
	#define NOMENUS           // MF_ *
	#define NOICONS           // IDI_ *
	#define NOKEYSTATES       // MK_ *
	#define NOSYSCOMMANDS     // SC_ *
	#define NORASTEROPS       // Binary and Tertiary raster ops
	#define NOSHOWWINDOW      // SW_ *
	#define OEMRESOURCE       // OEM Resource values
	#define NOATOM            // Atom Manager routines
	#define NOCLIPBOARD       // Clipboard routines
	#define NOCOLOR           // Screen colors
	#define NOCTLMGR          // Control and Dialog routines
	#define NODRAWTEXT        // DrawText() and DT_ *
	#define NOGDI             // All GDI defines androutines
	#define NOKERNEL          // All KERNEL defines androutines
//  #define NOUSER            // All USER defines androutines
	#define NONLS             // All NLS defines androutines
//	#define NOMB              // MB_ * andMessageBox()
	#define NOMEMMGR          // GMEM_ *, LMEM_ *, GHND, LHND, associated routines
	#define NOMETAFILE        // typedef METAFILEPICT
	#define NOMINMAX          // Macros min(a, b) and max(a, b)
	#define NOMSG             // typedef MSG andassociated routines
	#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, andOF_ *
	#define NOSCROLL          // SB_ * andscrolling routines
	#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
	#define NOSOUND           // Sound driver routines
	#define NOTEXTMETRIC      // typedef TEXTMETRIC andassociated routines
	#define NOWH              // SetWindowsHook and WH_ *
	#define NOWINOFFSETS      // GWL_ *, GCL_ *, associated routines
	#define NOCOMM            // COMM driver routines
	#define NOKANJI           // Kanji support stuff.
	#define NOHELP            // Help engine interface.
	#define NOPROFILER        // Profiler interface.
	#define NODEFERWINDOWPOS  // DeferWindowPos routines
	#define NOMCX             // Modem Configuration Extensions
	#define WIN32_LEAN_AND_MEAN // Cryptography, DDE, RPC, Shell, and Windows Sockets
	#include <Windows.h>
#endif

#include <algorithm>
#include <numeric>
#include <PreHeader.hpp>
#include <SystemsManager.hpp>
#include <SystemCreation.hpp>
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

#if !defined(DEBUG) && !defined(_DEBUG)
	#undef ASSUME
	#define ASSUME(cond) do { if (!(cond)) printf("Assumption %s failed, file %s line %i\n", TOSTR(cond), __FILE__, __LINE__); } while (false)
#endif