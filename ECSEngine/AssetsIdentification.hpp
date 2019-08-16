#pragma once

#include "AssetIdMapper.hpp"

namespace ECSEngine
{
	class MeshPathAssetIdentification : public AssetIdMapper::AssetIdentification
	{
	public:
		[[nodiscard]] static shared_ptr<MeshPathAssetIdentification> New(const FilePath &path, ui32 subMesh, f32 globalScale);
		[[nodiscard]] virtual bool operator == (const AssetIdentification &other) const override;
		[[nodiscard]] virtual uiw operator () () const override;
		[[nodiscard]] virtual TypeId AssetTypeId() const override;
		[[nodiscard]] const FilePath &AssetFilePath() const;
		[[nodiscard]] ui32 SubMeshIndex() const;
		[[nodiscard]] f32 GlobalScale() const;

	protected:
		MeshPathAssetIdentification(const FilePath &path, ui32 subMesh, f32 globalScale);

	private:
		FilePath _path{};
		ui32 _subMesh{};
		f32 _globalScale{};
	};
}