#pragma once

#include <TypeIdentifiable.hpp>

#ifdef USE_ID_NAMES
    #define IDINPUT , ui64 encoded0, ui64 encoded1, ui64 encoded2
    #define IDOUTPUT <stableId, encoded0, encoded1, encoded2>
#else
    #define IDINPUT
    #define IDOUTPUT <stableId>
#endif

namespace ECSTest
{
    enum RequirementForComponent
    {
        Required,
        Optional,
        Subtractive
    };

	class ComponentID
	{
    public:
        static constexpr ui32 invalidId = ui32_max;

    private:
		ui32 _id = invalidId;

	public:
		ComponentID() = default;
        explicit ComponentID(ui32 id);
        ui32 ID() const;
        bool IsValid() const;
        bool operator == (const ComponentID &other) const;
        bool operator != (const ComponentID &other) const;
        bool operator < (const ComponentID &other) const;
	};

    class ComponentIDGenerator
    {
    protected:
        std::atomic<ui32> _current = 0;

    public:
        ComponentID Generate();
        ComponentID LastGenerated() const;
        ComponentIDGenerator() = default;
        ComponentIDGenerator(ComponentIDGenerator &&source);
        ComponentIDGenerator &operator = (ComponentIDGenerator &&source);
    };

    class Component
    {
    };

    template <bool isUnique, ui64 stableId IDINPUT> class EMPTY_BASES _BaseComponent : public Component, public StableTypeIdentifiable IDOUTPUT
    {
    public:
        using StableTypeIdentifiable IDOUTPUT ::GetTypeId;

		[[nodiscard]] static constexpr bool IsUnique()
        {
            return isUnique;
        }
    };

#ifdef USE_ID_NAMES
    #define _CREATE_COMPONENT(name, isUnique) struct name final : public _BaseComponent<isUnique, Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>
#else
    #define _CREATE_COMPONENT(name, isUnique) struct name final : public _BaseComponent<isUnique, Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>
#endif

    // TODO: replace it with a single COMPONENT macro that optionally accepts uniqueness
    #define COMPONENT(name) _CREATE_COMPONENT(name, true)
    #define NONUNIQUE_COMPONENT(name) _CREATE_COMPONENT(name, false)

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}

namespace std
{
    template <> struct hash<ECSTest::ComponentID>
    {
        [[nodiscard]] size_t operator()(const ECSTest::ComponentID &value) const
        {
            return value.ID();
        }
    };
}

#undef IDINPUT
#undef IDOUTPUT