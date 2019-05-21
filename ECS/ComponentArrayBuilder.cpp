#include "PreHeader.hpp"
#include "ComponentArrayBuilder.hpp"

using namespace ECSTest;

void ComponentArrayBuilder::Clear()
{
    _components = {};
    _data = {};
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
    uiw copyIndex;

    if (sc.isTag == false)
    {
        ui8 *oldPtr = _data.data();
        _data.resize(_data.size() + _data.size() % sc.alignmentOf);
        copyIndex = _data.size();
        _data.resize(_data.size() + sc.sizeOf);
        ui8 *newPtr = _data.data();
        if (oldPtr != newPtr)
        {
            for (auto &stored : _components)
            {
                if (stored.data)
                {
                    if (newPtr > oldPtr)
                    {
                        stored.data += newPtr - oldPtr;
                    }
                    else
                    {
                        stored.data -= oldPtr - newPtr;
                    }
                }
            }
        }
    }
    else
    {
        ASSUME(sc.data == nullptr);
    }

    _components.push_back(sc);

    if (sc.isTag == false)
    {
        SerializedComponent &added = _components.back();
        MemOps::Copy(_data.data() + copyIndex, sc.data, sc.sizeOf);

        added.data = _data.data() + copyIndex;
    }

    return *this;
}