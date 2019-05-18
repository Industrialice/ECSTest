#pragma once

#include <TypeIdentifiable.hpp>

namespace ECSTest
{
    enum class RequirementForComponent
    {
        Required,
        RequiredWithData,
		Optional,
        OptionalWithData,
        Subtractive
    };

	struct ComponentDescription
	{
		StableTypeId type{};
		ui16 sizeOf{};
		ui16 alignmentOf{};
		bool isUnique{};
		bool isTag{};
	};

	struct ArchetypeDefiningRequirement
	{
		StableTypeId type{};
		ui32 group{};
		RequirementForComponent requirement{};
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
		[[nodiscard]] ui32 ID() const;
		[[nodiscard]] bool IsValid() const;
		[[nodiscard]] bool operator == (const ComponentID &other) const;
		[[nodiscard]] bool operator != (const ComponentID &other) const;
		[[nodiscard]] bool operator < (const ComponentID &other) const;
		[[nodiscard]] bool operator <= (const ComponentID &other) const;
		[[nodiscard]] bool operator > (const ComponentID &other) const;
		[[nodiscard]] bool operator >= (const ComponentID &other) const;
		[[nodiscard]] explicit operator bool() const;
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

    template <bool isUnique, bool isTag, typename Type, typename FinalType> class EMPTY_BASES _BaseComponent : public Component, public Type
    {
    public:
        using Type::GetTypeId;
        using Type::GetTypeName;

		[[nodiscard]] static constexpr bool IsUnique()
        {
            return isUnique;
        }

        [[nodiscard]] static constexpr bool IsTag()
        {
            return isTag;
        }

		[[nodiscard]] static constexpr ComponentDescription Description()
		{
			ComponentDescription desc;
			desc.alignmentOf = alignof(FinalType);
			desc.isTag = isTag;
			desc.isUnique = isUnique;
			desc.sizeOf = sizeof(FinalType);
			desc.type = GetTypeId();
			return desc;
		}
    };

	#define _CREATE_COMPONENT(name, isUnique, isTag) struct name final : public _BaseComponent<isUnique, isTag, NAME_TO_STABLE_ID(name), name>

    // TODO: replace it with a single COMPONENT macro that optionally accepts properties?
    #define COMPONENT(name) _CREATE_COMPONENT(name, true, false)
    #define NONUNIQUE_COMPONENT(name) _CREATE_COMPONENT(name, false, false)
    #define TAG_COMPONENT(name) _CREATE_COMPONENT(name, true, true) {}

	struct _SubtractiveComponentBase
	{};

	template <typename... Types> struct EMPTY_BASES SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentTypes = tuple<Types...>;
	};

    struct _RequiredComponentBase
    {};

    template <typename... Types> struct EMPTY_BASES RequiredComponent : _RequiredComponentBase
    {
        using ComponentTypes = tuple<Types...>;
    };

	struct _OptionalComponentBase
	{};

	template <typename... Types> struct EMPTY_BASES OptionalComponent : _OptionalComponentBase
	{
		using ComponentTypes = tuple<Types...>;
	};

	struct _RequiredComponentAnyBase
	{};

	template <typename... Types> struct EMPTY_BASES RequiredComponentAny : _RequiredComponentAnyBase
	{
		using ComponentTypes = tuple<Types...>;
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