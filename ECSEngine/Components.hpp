#pragma once

namespace ECSEngine
{
    COMPONENT(Position)
    {
        Vector3 position;
    };

    COMPONENT(Rotation)
    {
        Quaternion rotation;
    };

    COMPONENT(Scale)
    {
        Vector3 scale;
    };

    COMPONENT(Parent)
    {
        EntityID parent;
    };

    COMPONENT(MeshRenderer)
    {};

    COMPONENT(SkinnedMeshRenderer)
    {};

    struct Window
    {
        enum class CursorTypet : ui8
        {
            Normal, Busy, Hand, Target, No, Help
        };

        i32 width{}, height{}; // they are signed so you wounldn't have missmatches when combining windows' sizes with x/y
        i32 x{}, y{};
        i16 dpiX{}, dpiY{};
        std::array<char, 32> title{};
        bool isFullscreen = false;
        bool isNoBorders = isFullscreen;
        bool isMaximized = !isFullscreen;
        CursorTypet cursorType{};
    };

    struct RT
    {
        variant<std::monostate, Window> target{};
    };

    COMPONENT(Camera)
    {
        f32 fov;
        f32 size;
        bool isClearDepth;
        f32 nearPlane;
        f32 farPlane;
        RT rts[8];
        enum class ProjectionType : ui8 { Perspective, Orthographic } projectionType;
    };
}