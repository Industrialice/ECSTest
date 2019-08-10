#pragma once

#include <AssetId.hpp>

namespace ECSEngine
{
	class AssetIdMapper
	{
		unique_ptr<IFile> _idMapFile{};
		ui32 _currentId = 0;
		std::unordered_map<AssetId, FilePath> _idMap{};

	public:
		void SetIdMapSource(unique_ptr<IFile> file); // file must be open for both read and write
		AssetId Register(const FilePath &pathToAsset); // will return the same id when called multiple times with the same path
		[[nodiscard]] optional<FilePath> ResolveIdToPath(AssetId id) const;
	};
}