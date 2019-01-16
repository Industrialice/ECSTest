#include "PreHeader.hpp"
#include "MessageBuilder.hpp"

using namespace ECSTest;

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const EntitiesStream::ComponentDesc &desc, ui32 id) -> ComponentArrayBuilder &
{
    SerializedComponent serialized;
    serialized.alignmentOf = desc.alignmentOf;
    serialized.data = desc.data;
    serialized.id = id;
    serialized.isUnique = desc.isUnique;
    serialized.sizeOf = desc.sizeOf;
    serialized.type = desc.type;
    return AddComponent(serialized);
}

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const SerializedComponent &serializedComponent) -> ComponentArrayBuilder &
{
    // TODO: insert return statement here
}

void MessageBuilder::EntityRemoved(ArchetypeShort archetype, EntityID entityID)
{}

auto ECSTest::MessageBuilder::EntityAdded(EntityID entityID) -> ComponentArrayBuilder &
{
    // TODO: insert return statement here
}
