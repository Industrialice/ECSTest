#pragma once

#include "AssetIdMapper.hpp"

namespace ECSEngine
{
	class MeshPathAssetIdentification : public AssetIdMapper::AssetIdentification
	{
	public:
		[[nodiscard]] static shared_ptr<MeshPathAssetIdentification> New(const FilePath &path, ui32 subMesh, f32 globalScale, bool isUseFileScale);
		[[nodiscard]] virtual bool operator == (const AssetIdentification &other) const override;
		[[nodiscard]] virtual uiw operator () () const override;
		[[nodiscard]] virtual TypeId AssetTypeId() const override;
		[[nodiscard]] const FilePath &AssetFilePath() const;
		[[nodiscard]] ui32 SubMeshIndex() const;
		[[nodiscard]] f32 GlobalScale() const;
		[[nodiscard]] bool IsUseFileScale() const;

	protected:
		MeshPathAssetIdentification(const FilePath &path, ui32 subMesh, f32 globalScale, bool isUseFileScale);

	private:
		FilePath _path{};
		ui32 _subMesh = 0;
		f32 _globalScale = 1.0f;
		bool _isUseFileScale = true;
	};
}