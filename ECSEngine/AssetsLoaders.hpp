#pragma once

#include "AssetIdMapper.hpp"

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
			else
			{
				static_assert(false_v<type>, "Unknown asset type");
			}

			UNREACHABLE;
			return {};
		}

		[[nodiscard]] AssetsManager::AssetLoaderFuncType GenerateMeshLoaderFunction();
		[[nodiscard]] AssetsManager::AssetLoaderFuncType GenerateTextureLoaderFunction();
		void RegisterLoaders(AssetsManager &manager);
		void SetAssetIdMapper(const shared_ptr<AssetIdMapper> &mapper);
		void SetAssetsLocation(FilePath &&path);

	private:
		[[nodiscard]] AssetsManager::LoadedAsset LoadMesh(AssetId id, TypeId expectedType);
		[[nodiscard]] AssetsManager::LoadedAsset LoadTexture(AssetId id, TypeId expectedType);

		shared_ptr<AssetIdMapper> _assetIdMapper{};
		FilePath _assetsLocation{};
	};
}