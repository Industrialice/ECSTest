#pragma once

namespace ECSTest
{
	class AssetId : public OpaqueID<ui32, ui32_max>
	{
		using OpaqueID::OpaqueID;
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