#pragma once

#include <AssetId.hpp>

namespace ECSEngine
{
	class AssetIdMapper
	{
		std::unordered_map<FilePath, pair<AssetId, TypeId>> _assetPathToId{};
		std::unordered_map<AssetId, pair<FilePath, TypeId>> _assetIdToPath{};
		AssetId::idType _currentAssetId{};

	public:
		template <typename AssetType> typename AssetType::assetIdType Register(const FilePath &pathToAsset)
		{
			return Register(pathToAsset, AssetType::GetTypeId()).Cast<AssetType::assetIdType>();
		}
		AssetId Register(const FilePath &pathToAsset, TypeId assetType); // will return the same id when called multiple times with the same path
		[[nodiscard]] optional<pair<FilePath, TypeId>> ResolveIdToPath(AssetId id) const;
	};
}