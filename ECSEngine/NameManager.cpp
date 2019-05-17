#include "PreHeader.hpp"
#include "NameManager.hpp"
#include <DIWRSpinLock.hpp>

using namespace ECSEngine;

class NameManagerImpl : public NameManager
{
	static constexpr char defaultName[] = "GameObject";
	std::unordered_map<EntityID, string> _names{};

	virtual string_view NameOf(EntityID id) const override final
	{
		auto it = _names.find(id);
		if (it == _names.end())
		{
			return defaultName;
		}
		return it->second;
	}
	
	virtual void NameOf(EntityID id, string_view name) override final
	{
		if (name != defaultName)
		{
			_names[id] = name;
		}
	}
};

shared_ptr<NameManager> NameManager::New()
{
	struct Proxy final : public NameManagerImpl
	{
		using NameManagerImpl::NameManagerImpl;
	};
	return shared_ptr<Proxy>();
}
