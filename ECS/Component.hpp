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
        RequiredWithData,
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
    {};

    template <bool isUnique, bool isTag, ui64 stableId IDINPUT> class EMPTY_BASES _BaseComponent : public Component, public StableTypeIdentifiable IDOUTPUT
    {
    public:
        using StableTypeIdentifiable IDOUTPUT ::GetTypeId;

		[[nodiscard]] static constexpr bool IsUnique()
        {
            return isUnique;
        }

        [[nodiscard]] static constexpr bool IsTag()
        {
            return isTag;
        }
    };

#ifdef USE_ID_NAMES
    #define _CREATE_COMPONENT(name, isUnique, isTag) struct name final : public _BaseComponent<isUnique, isTag, Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>
#else
    #define _CREATE_COMPONENT(name, isUnique, isTag) struct name final : public _BaseComponent<isUnique, isTag, Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>
#endif

    // TODO: replace it with a single COMPONENT macro that optionally accepts properties?
    #define COMPONENT(name) _CREATE_COMPONENT(name, true, false)
    #define NONUNIQUE_COMPONENT(name) _CREATE_COMPONENT(name, false, false)
    #define TAG_COMPONENT(name) _CREATE_COMPONENT(name, true, true) {}

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct EMPTY_BASES SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};

    struct _RequiredComponentBase
    {};

    template <typename T> struct EMPTY_BASES RequiredComponent : _RequiredComponentBase
    {
        using ComponentType = T;
    };

	struct _NonUniqueBase
	{};

	template <typename T> struct EMPTY_BASES NonUnique : _NonUniqueBase
	{
		Array<T> components{};
		const Array<ComponentID> ids{};
		const ui32 stride{};

		NonUnique(Array<T> components, Array<ComponentID> ids, ui32 stride) : components(components), ids(ids), stride(stride)
		{}
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