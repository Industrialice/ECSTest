#pragma once

#ifndef _ENABLE_EXTENDED_ALIGNED_STORAGE
	#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#endif

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

#include <StdMiscLib.hpp>
#include <DIWRSpinLock.hpp>
#include <MatrixMathTypes.hpp>
#include <FunctionInfo.hpp>
#include <TimeMoment.hpp>
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
using std::byte;
using namespace std::literals;
using namespace std::placeholders;

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

#ifdef PLATFORM_ANDROID
	void DetachCurrentThread();
#endif

template <typename T, T InvalidIDValue> class OpaqueID
{
protected:
	T _id = InvalidIDValue;
	#ifdef DEBUG
		char _debugName[64]{};
	#endif

	using hashType = conditional_t<sizeof(T) == 1, ui8,
		conditional_t<sizeof(T) == 2, ui16,
		conditional_t<sizeof(T) == 4, ui32, ui64>>>;

public:	
	using idType = T;

	OpaqueID() = default;
	
	explicit OpaqueID(T id) : _id(id)
	{}

	[[nodiscard]] hashType Hash() const
	{
		return static_cast<hashType>(_id);
	}

	[[nodiscard]] bool operator == (const OpaqueID &other) const
	{
		return _id == other._id;
	}
	
	[[nodiscard]] bool operator != (const OpaqueID &other) const
	{
		return _id != other._id;
	}

	[[nodiscard]] bool operator < (const OpaqueID &other) const
	{
		return _id < other._id;
	}

	[[nodiscard]] bool operator <= (const OpaqueID &other) const
	{
		return _id <= other._id;
	}

	[[nodiscard]] bool operator > (const OpaqueID &other) const
	{
		return _id > other._id;
	}

	[[nodiscard]] bool operator >= (const OpaqueID &other) const
	{
		return _id >= other._id;
	}

	[[nodiscard]] bool IsValid() const
	{
		return _id != InvalidIDValue;
	}
	
	[[nodiscard]] explicit operator bool() const
	{
		return IsValid();
	}

	[[nodiscard]] static constexpr T InvalidID()
	{
		return InvalidIDValue;
	}

	void DebugName(const char *name)
	{
		#ifdef DEBUG
			return DebugName(string_view(name, strlen(name)));
		#endif
	}

	void DebugName(string_view name)
	{
		#ifdef DEBUG
			uiw copyLen = std::min(CountOf(_debugName) - 1, name.size());
			if (copyLen < name.size())
			{
				name = name.substr(name.size() - copyLen, copyLen);
			}
			MemOps::Copy(_debugName, name.data(), copyLen);
			_debugName[copyLen] = '\0';
		#endif
	}

	StringViewNullTerminated DebugName() const
	{
		#ifdef DEBUG
			return string_view(_debugName, strlen(_debugName));
		#else
			return {};
		#endif
	}
};