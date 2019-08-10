#include "PreHeader.hpp"
#include "AssetIdMapper.hpp"

using namespace ECSEngine;

void AssetIdMapper::SetIdMapSource(unique_ptr<IFile> file)
{
}

AssetId AssetIdMapper::Register(const FilePath &pathToAsset)
{
	return AssetId();
}

optional<FilePath> AssetIdMapper::ResolveIdToPath(AssetId id) const
{
	return optional<FilePath>();
}
