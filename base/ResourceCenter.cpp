#include <atomic>
#include <map>
#include <set>
#include "ResourceCenter.h"

using namespace fpnn;

static std::mutex _mutex;
static std::atomic<bool> _created(false);
static ResourceCenterPtr _resourceCenter;

ResourceCenter::Guard::Guard(): _lock(_mutex) {}

ResourceCenter::~ResourceCenter()
{
	std::map<int, std::set<IResourcePtr>> cache;
	for (auto& hpr: _holder)
		cache[hpr.second.releaseOrder].insert(hpr.second.resource);

	_holder.clear();

	for (auto it = cache.rbegin(); it != cache.rend(); it++)
		it->second.clear();
}

ResourceCenterPtr ResourceCenter::instance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_resourceCenter.reset(new ResourceCenter);
			_created = true;
		}
	}
	return _resourceCenter;
}

bool ResourceCenter::add(const std::string& key, IResourcePtr resource, int releaseOrder)
{
	ResourcePackage package;
	package.releaseOrder = releaseOrder;
	package.resource = resource;

	ResourceCenterPtr inst = instance();
	std::unique_lock<std::mutex> lck(_mutex);
	auto it = inst->_holder.find(key);
	if (it == inst->_holder.end())
	{
		inst->_holder[key] = package;
		return true;
	}
	else
		return false;
}

IResourcePtr ResourceCenter::get(const std::string& key)
{
	ResourceCenterPtr inst = instance();
	std::unique_lock<std::mutex> lck(_mutex);
	auto it = inst->_holder.find(key);
	if (it != inst->_holder.end())
		return it->second.resource;
	else
		return nullptr;
}

int ResourceCenter::getReleaseOrder(const std::string& key)
{
	ResourceCenterPtr inst = instance();
	std::unique_lock<std::mutex> lck(_mutex);
	auto it = inst->_holder.find(key);
	if (it != inst->_holder.end())
		return it->second.releaseOrder;
	else
		return 0;
}

void ResourceCenter::erase(const std::string& key)
{
	ResourceCenterPtr inst = instance();
	std::unique_lock<std::mutex> lck(_mutex);
	inst->_holder.erase(key);
}
