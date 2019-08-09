#pragma once

namespace ECSEngine
{
	class AssetsLoaders
	{
	public:
		template <TypeId::idType type> AssetsManager::AssetLoaderFuncType GenerateLoaderFunction()
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

		AssetsManager::AssetLoaderFuncType GenerateMeshLoaderFunction();
		AssetsManager::AssetLoaderFuncType GenerateTextureLoaderFunction();
		void RegisterLoaders(AssetsManager &manager);

	private:
		AssetsManager::LoadedAsset LoadMesh(AssetId id, TypeId expectedType);
		AssetsManager::LoadedAsset LoadTexture(AssetId id, TypeId expectedType);
	};
}