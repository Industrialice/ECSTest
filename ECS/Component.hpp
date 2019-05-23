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
		TypeId type{};
		ui16 sizeOf{};
		ui16 alignmentOf{};
		bool isUnique{};
		bool isTag{};
	};

	struct ArchetypeDefiningRequirement
	{
		TypeId type{};
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
#ifdef SPACESHIP_SUPPORTED
		[[nodiscard]] auto operator <=> (const ComponentID &other) const = default;
#else
		[[nodiscard]] bool operator == (const ComponentID &other) const;
		[[nodiscard]] bool operator != (const ComponentID &other) const;
		[[nodiscard]] bool operator < (const ComponentID &other) const;
		[[nodiscard]] bool operator <= (const ComponentID &other) const;
		[[nodiscard]] bool operator > (const ComponentID &other) const;
		[[nodiscard]] bool operator >= (const ComponentID &other) const;
#endif
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

    class _BaseComponentClass
    {};

    template <bool isUnique, bool isTag, typename ComponentType> class EMPTY_BASES _BaseComponent : public _BaseComponentClass, public TypeIdentifiable<ComponentType>
    {
    public:
        using TypeIdentifiable<ComponentType>::GetTypeId;
        using TypeIdentifiable<ComponentType>::GetTypeName;

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
			desc.alignmentOf = alignof(ComponentType);
			desc.isTag = isTag;
			desc.isUnique = isUnique;
			desc.sizeOf = sizeof(ComponentType);
			desc.type = GetTypeId();
			return desc;
		}
    };

	template <typename ComponentType> struct EMPTY_BASES Component : public _BaseComponent<true, false, ComponentType>
	{};

	template <typename ComponentType> struct EMPTY_BASES TagComponent : public _BaseComponent<true, true, ComponentType>
	{};

	template <typename ComponentType> struct EMPTY_BASES NonUniqueComponent : public _BaseComponent<false, false, ComponentType>
	{};

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