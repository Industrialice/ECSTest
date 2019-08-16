#pragma once

#include <AssetId.hpp>

namespace ECSEngine
{
	class AssetIdMapper
	{
	public:
		class AssetIdentification
		{
		public:
			virtual ~AssetIdentification() = default;
			[[nodiscard]] virtual bool operator == (const AssetIdentification &other) const = 0;
			[[nodiscard]] virtual uiw operator () () const = 0;
			[[nodiscard]] virtual TypeId AssetTypeId() const = 0;
		};

		template <typename AssetType> typename AssetType::assetIdType Register(const shared_ptr<AssetIdentification> &assetIdentification)
		{
			return Register(assetIdentification).Cast<AssetType::assetIdType>();
		}
		AssetId Register(const shared_ptr<AssetIdentification> &assetIdentification); // will return the same id when called multiple times with the same identification
		[[nodiscard]] const AssetIdentification *Resolve(AssetId id) const;

	private:
		struct Hasher
		{
			uiw operator () (const shared_ptr<AssetIdentification> &value) const
			{
				return value->operator()();
			}
		};

		struct Comparator
		{
			bool operator () (const shared_ptr<AssetIdentification> &left, const shared_ptr<AssetIdentification> &right) const
			{
				return left->operator==(*right);
			}
		};

		std::unordered_map<shared_ptr<AssetIdentification>, pair<AssetId, TypeId>, Hasher, Comparator> _assetPathToId{};
		std::unordered_map<AssetId, shared_ptr<AssetIdentification>> _assetIdToPath{};
		AssetId::idType _currentAssetId{};
	};
}