#pragma once

#include "AssetIdMapper.hpp"
#include "PhysicsProperties.hpp"

namespace ECSEngine
{
	class MeshPathAssetIdentification : public AssetIdMapper::AssetIdentification
	{
	public:
		[[nodiscard]] static shared_ptr<MeshPathAssetIdentification> New(const FilePath &path, optional<ui8> subMesh, f32 globalScale, bool isUseFileScale);
		[[nodiscard]] virtual bool operator == (const AssetIdentification &other) const override;
		[[nodiscard]] virtual uiw operator () () const override;
		[[nodiscard]] virtual TypeId AssetTypeId() const override;
		[[nodiscard]] const FilePath &AssetFilePath() const;
		[[nodiscard]] optional<ui8> SubMeshIndex() const;
		[[nodiscard]] f32 GlobalScale() const;
		[[nodiscard]] bool IsUseFileScale() const;

	protected:
		MeshPathAssetIdentification(const FilePath &path, optional<ui8> subMesh, f32 globalScale, bool isUseFileScale);

	private:
		FilePath _path{};
		optional<ui8> _subMesh{};
		f32 _globalScale = 1.0f;
		bool _isUseFileScale = true;
	};

	class PhysicsPropertiesAssetIdentification : public AssetIdMapper::AssetIdentification
	{
	public:
		[[nodiscard]] static shared_ptr<PhysicsPropertiesAssetIdentification> New(PhysicsProperties &&properties);
		[[nodiscard]] virtual bool operator == (const AssetIdentification &other) const override;
		[[nodiscard]] virtual uiw operator () () const override;
		[[nodiscard]] virtual TypeId AssetTypeId() const override;
		[[nodiscard]] const PhysicsProperties &Properties() const;

	protected:
		PhysicsPropertiesAssetIdentification(PhysicsProperties &&properties);

	private:
		PhysicsProperties _properties{};
	};
}