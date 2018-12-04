#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <stdio.h>

using namespace ECSTest;

class GameEntities : public EntitiesStream
{
public:
    [[nodiscard]] virtual optional<StreamedEntity> Next() override
    {
        return {};
    }
};

void main()
{
    StdLib::Initialization::Initialize({});

    printf("done\n");
    getchar();
}