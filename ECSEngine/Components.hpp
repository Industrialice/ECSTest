#pragma once

#include "Color.hpp"

namespace ECSEngine
{
    COMPONENT(Position)
    {
		Vector3 position = {0, 0, 0};
    };

    COMPONENT(Rotation)
    {
		Quaternion rotation{};
    };

    COMPONENT(Scale)
    {
		Vector3 scale = {1, 1, 1};
    };

    COMPONENT(Parent)
    {
		EntityID parent{};
    };

	TAG_COMPONENT(HasChildren);

    COMPONENT(MeshRenderer)
    {
		ui32 index = 0;
	};

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
        bool isFullscreen = false;
        bool isNoBorders = isFullscreen;
        bool isMaximized = !isFullscreen;
        CursorTypet cursorType{};
        array<char, 64> title{};
    };

    struct ClearColor
    {
        ColorR8G8B8 color{};
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
        variant<std::monostate, ClearColor> clearWith;
        array<RT, 8> rt;
        enum class ProjectionType : ui8 { Perspective, Orthographic } projectionType;
    };
}