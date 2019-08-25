#pragma once

namespace ECSTest
{
	class AssetId : public OpaqueID<ui16, ui16_max>
	{
		using OpaqueID::OpaqueID;

	public:
		template <typename TargetAssetType, typename = enable_if_t<is_base_of_v<AssetId, TargetAssetType>>> [[nodiscard]] TargetAssetType Cast() const
		{
			return TargetAssetType(_id);
		}
	};
}

namespace std
{
	template <> struct hash<ECSTest::AssetId>
	{
		[[nodiscard]] size_t operator()(const ECSTest::AssetId &value) const
		{
			return value.Hash();
		}
	};
}