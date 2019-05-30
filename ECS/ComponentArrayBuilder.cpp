#include "PreHeader.hpp"
#include "ComponentArrayBuilder.hpp"

using namespace ECSTest;

void ComponentArrayBuilder::Clear()
{
    _components.clear();
    _data.clear();
}

auto ComponentArrayBuilder::AddComponent(const IEntitiesStream::ComponentDesc &desc, ComponentID id) -> ComponentArrayBuilder &
{
    SerializedComponent serialized;
    serialized.alignmentOf = desc.alignmentOf;
    serialized.data = desc.data;
    serialized.id = id;
    serialized.isUnique = desc.isUnique;
    serialized.isTag = desc.isTag;
    serialized.sizeOf = desc.sizeOf;
    serialized.type = desc.type;
    return AddComponent(serialized);
}

auto ComponentArrayBuilder::AddComponent(const SerializedComponent &sc) -> ComponentArrayBuilder &
{
	_components.push_back(sc);
	if (sc.isTag)
	{
		return *this;
	}

	unique_ptr<byte[], AlignedMallocDeleter> data(Allocator::MallocAlignedRuntime::Allocate(sc.sizeOf, sc.alignmentOf));
	MemOps::Copy(data.get(), sc.data, sc.sizeOf);

    SerializedComponent &added = _components.back();
	added.data = data.get();

	_data.emplace_back(move(data));

	return *this;
}