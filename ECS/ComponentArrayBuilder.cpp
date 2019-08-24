#include "PreHeader.hpp"
#include "ComponentArrayBuilder.hpp"

using namespace ECSTest;

ComponentArrayBuilder::ComponentArrayBuilder(const ComponentArrayBuilder &source)
{
	_components = source._components;
	_data.resize(source._data.size());
	for (uiw index = 0, dataIndex = 0; index < _components.size(); ++index)
	{
		if (_components[index].isTag)
		{
			continue;
		}
		_data[dataIndex].reset(Allocator::MallocAlignedRuntime::Allocate(_components[index].sizeOf, _components[index].alignmentOf));
		MemOps::Copy(_data[dataIndex].get(), source._data[dataIndex].get(), _components[index].sizeOf);
		++dataIndex;
	}
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

void ComponentArrayBuilder::Clear()
{
	_components.clear();
	_data.clear();
}

Array<const SerializedComponent> ComponentArrayBuilder::GetComponents() const
{
	return ToArray(_components);
}
