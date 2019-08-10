#pragma once

#include "AssetId.hpp"

namespace ECSTest 
{
	class AssetsManager
	{
	public:
		struct LoadedAsset
		{
			TypeId type;
			shared_ptr<void> data;
		};

		using AssetLoaderFuncType = std::function<LoadedAsset(AssetId id, TypeId expectedType)>;

	private:
		std::unordered_map<AssetId, LoadedAsset> _loadedAssets{};
		std::unordered_map<TypeId, AssetLoaderFuncType> _assetLoaders{};

	public:
		template <typename T, typename = decltype(T::GetTypeId)> const T *Load(AssetId id)
		{
			auto loadedAssetIt = _loadedAssets.find(id);
			if (loadedAssetIt != _loadedAssets.end())
			{
				if (loadedAssetIt->second.type != T::GetTypeId())
				{
					SOFTBREAK;
					return nullptr;
				}
				return static_cast<const T *>(loadedAssetIt->second.data.get());
			}

			auto assetLoaderIt = _assetLoaders.find(T::GetTypeId());
			if (assetLoaderIt == _assetLoaders.end())
			{
				SOFTBREAK;
				return nullptr;
			}

			auto loadedAsset = assetLoaderIt->second(id, T::GetTypeId());
			if (loadedAsset.data == nullptr)
			{
				SOFTBREAK;
				return nullptr;
			}

			ASSUME(loadedAsset.type == T::GetTypeId());

			_loadedAssets[id] = loadedAsset;

			return static_cast<const T *>(loadedAsset.data.get());
		}

		void AddAssetLoader(TypeId type, AssetLoaderFuncType &&func);
		void RemoveAssetLoader(TypeId type);
	};
}