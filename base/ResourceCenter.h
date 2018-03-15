#ifndef FPNN_Resource_Center_h
#define FPNN_Resource_Center_h

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace fpnn
{
	class ResourceCenter;
	typedef std::shared_ptr<ResourceCenter> ResourceCenterPtr;

	class IResource
	{
	public:
		virtual ~IResource() {}
	};
	typedef std::shared_ptr<IResource> IResourcePtr;

	class ResourceCenter
	{
	public:
		class Guard
		{
			std::unique_lock<std::mutex> _lock;
		public:
			Guard();
			virtual ~Guard() {}
		};

	private:
		struct ResourcePackage
		{
			int releaseOrder;
			IResourcePtr resource;

			ResourcePackage(): releaseOrder(0) {}
		};

	private:
		std::unordered_map<std::string, ResourcePackage> _holder;

	public:
		static ResourceCenterPtr instance();
		//-- releaseOrder: bigger is first, smaller is last.
		static bool add(const std::string& key, IResourcePtr resource, int releaseOrder = 0);
		static IResourcePtr get(const std::string& key);
		static int getReleaseOrder(const std::string& key);
		static void erase(const std::string& key);

		~ResourceCenter();
	};
}
#endif
