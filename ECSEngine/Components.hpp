#pragma once

#include "Color.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "PhysicsProperties.hpp"

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

	struct LinearVelocity : Component<LinearVelocity>
	{
		Vector3 velocity{};
	};

	struct AngularVelocity : Component<AngularVelocity>
	{
		Vector3 velocity{};
	};

    struct Parent : Component<Parent>
    {
		EntityID parent{};
    };

	struct HasChildren : TagComponent<HasChildren> {};

    struct MeshRenderer : NonUniqueComponent<MeshRenderer>
    {
		MeshAssetId mesh{};
		MaterialAssetsArrayAssetId materials{};
	};

    struct SkinnedMeshRenderer : Component<SkinnedMeshRenderer>
    {
		char : 8;
	};

	struct BoxCollider : Component<BoxCollider>
	{
		bool isTrigger = false;
		Vector3 center = {0, 0, 0};
		Vector3 size = {1, 1, 1};
		Quaternion rotation{};
	};

	struct SphereCollider : Component<SphereCollider>
	{
		bool isTrigger = false;
		Vector3 center = {0, 0, 0};
		f32 radius = 1;
	};

	struct CapsuleCollider : Component<CapsuleCollider>
	{
		enum class Direction { X, Y, Z };

		bool isTrigger = false;
		Vector3 center = {0, 0, 0};
		f32 radius = 1;
		f32 height = 1;
		Direction direction{Direction::X};
	};

	struct MeshCollider : Component<MeshCollider>
	{
		bool isTrigger = false;
		ui8 vertexLimit = 255;
		MeshAssetId mesh{};
	};

	struct TerrainCollider : Component<TerrainCollider>
	{
	};

	struct Physics : Component<Physics>
	{
		PhysicsPropertiesAssetId physics{};
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

		enum class DepthBufferFormat : ui8
		{
			Neither,
			DepthOnly,
			DepthAndStencil
		};

		f32 fov{};
		f32 size{};
		bool isClearDepthStencil{};
		DepthBufferFormat depthBufferFormat{};
		f32 nearPlane{};
		f32 farPlane{};
		variant<std::monostate, ClearColor> clearWith{};
		array<RT, 8> rt{};
		ProjectionTypet projectionType{};
    };
}