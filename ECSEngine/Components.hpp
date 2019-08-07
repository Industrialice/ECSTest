#pragma once

#include "Color.hpp"

namespace ECSEngine
{
    struct Position : Component<Position>
    {
		Vector3 position = {0, 0, 0};
    };

    struct Rotation : Component<Rotation>
    {
		Quaternion rotation{};
    };

    struct Scale : Component<Scale>
    {
		Vector3 scale = {1, 1, 1};
    };

    struct Parent : Component<Parent>
    {
		EntityID parent{};
    };

	struct HasChildren : TagComponent<HasChildren> {};

    struct MeshRenderer : Component<MeshRenderer>
    {
		ui32 index = 0;
	};

    struct SkinnedMeshRenderer : Component<SkinnedMeshRenderer>
    {
		char nullChar = '\0';
	};

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

    struct Camera : Component<Camera>
    {
		enum class ProjectionTypet : ui8
		{
			Perspective,
			Orthographic
		};

		f32 fov{};
		f32 size{};
		bool isClearDepth{};
		f32 nearPlane{};
		f32 farPlane{};
		variant<std::monostate, ClearColor> clearWith{};
		array<RT, 8> rt{};
		ProjectionTypet projectionType{};
    };
}