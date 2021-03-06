#pragma once

#include "AssetIdMapper.hpp"
#include "Mesh.hpp"
#include "PhysicsProperties.hpp"

namespace ECSEngine
{
	class AssetsLoaders
	{
	public:
		template <TypeId::idType type> [[nodiscard]] AssetsManager::AssetLoaderFuncType GenerateLoaderFunction()
		{
			if constexpr (type == MeshAsset::GetTypeId())
			{
				return GenerateMeshLoaderFunction();
			}
			else if constexpr (type == TextureAsset::GetTypeId())
			{
				return GenerateTextureLoaderFunction();
			}
			else if constexpr (type == PhysicsPropertiesAsset::GetTypeId())
			{
				return GeneratePhysicsPropertiesLoaderFunction();
			}
			else
			{
				static_assert(false_v<type>, "Unknown asset type");
			}

			UNREACHABLE;
			return {};
		}

		[[nodiscard]] AssetsManager::AssetLoaderFuncType GenerateMeshLoaderFunction();
		[[nodiscard]] AssetsManager::AssetLoaderFuncType GenerateTextureLoaderFunction();
		[[nodiscard]] AssetsManager::AssetLoaderFuncType GeneratePhysicsPropertiesLoaderFunction();
		void RegisterLoaders(AssetsManager &manager);
		void SetAssetIdMapper(const shared_ptr<AssetIdMapper> &mapper);

	private:
		[[nodiscard]] AssetsManager::LoadedAsset LoadMesh(AssetId id, TypeId expectedType);
		[[nodiscard]] AssetsManager::LoadedAsset LoadTexture(AssetId id, TypeId expectedType);
		[[nodiscard]] AssetsManager::LoadedAsset LoadPhysicsProperties(AssetId id, TypeId expectedType);

		shared_ptr<AssetIdMapper> _assetIdMapper{};
	};
}