#pragma once

#ifdef _MSC_VER
    #include "WinAPI.hpp"
    #include <d3d11.h>
	#include <d3dcompiler.h>
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <PreHeader.hpp>
#include <forward_list>
#include <unordered_set>
#include <SystemsManager.hpp>
#include <KeyController.hpp>

using namespace ECSTest;

inline shared_ptr<Logger<string_view, true>> EngineLogger{};

#ifdef DEBUG
    // this snprintf will trigger compiler warnings if incorrect args were passed in
    #define SENDLOG(level, sender, ...) snprintf(nullptr, 0, __VA_ARGS__); EngineLogger->Message(StdLib::LogLevels:: level, TOSTR(sender), __VA_ARGS__)
#else
    #define SENDLOG(level, sender, ...) EngineLogger->Message(StdLib::LogLevels:: level, TOSTR(sender), __VA_ARGS__)
#endif

template <typename T, typename Deleter> T **AddressOfNaked(unique_ptr<T, Deleter> &ptr)
{
	static_assert(sizeof(unique_ptr<T, Deleter>) == sizeof(T *), "Unsafe operation");
	return reinterpret_cast<T **>(&ptr);
}