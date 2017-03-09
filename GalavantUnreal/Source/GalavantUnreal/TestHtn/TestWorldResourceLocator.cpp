#include "GalavantUnreal.h"

#include "TestWorldResourceLocator.hpp"
#include <limits>

TestWorldResourceLocator::~TestWorldResourceLocator()
{
	for (std::pair<const WorldResourceType, ResourceList*>& resourceTypeList : Resources)
	{
		delete resourceTypeList.second;
	}
	Resources.clear();
}

bool TestWorldResourceLocator::ResourceListExists(const WorldResourceType type) const
{
	return Resources.find(type) != Resources.end();
}

bool TestWorldResourceLocator::ResourceExistsInWorld(const WorldResourceType type)
{
	return ResourceListExists(type) && !Resources[type]->empty();
}

void TestWorldResourceLocator::AddResource(const WorldResourceType type,
                                           const gv::Position& location)
{
	gv::Position newResource(location);
	// Ensure we're not exactly 0,0,0 because I designed this poorly
	newResource.Z = !newResource ? 0.1f : newResource.Z;

	if (ResourceListExists(type))
	{
		Resources[type]->push_back(newResource);
	}
	else
	{
		ResourceList* newResourceList = new ResourceList;
		newResourceList->push_back(newResource);
		Resources[type] = newResourceList;
	}
}

void TestWorldResourceLocator::RemoveResource(const WorldResourceType type,
                                              const gv::Position& location)
{
	if (ResourceListExists(type))
	{
		ResourceList* resourceList = Resources[type];
		ResourceList::iterator resourceIt =
		    std::find(resourceList->begin(), resourceList->end(), location);
		if (resourceIt != resourceList->end())
			resourceList->erase(resourceIt);
	}
}

void TestWorldResourceLocator::MoveResource(const WorldResourceType type,
                                            const gv::Position& oldLocation,
                                            const gv::Position& newLocation)
{
	if (ResourceListExists(type))
	{
		for (gv::Position& currentResource : *Resources[type])
		{
			// They should be exactly equal. It's the caller's responsibility to keep track of this
			if (currentResource.Equals(oldLocation, 0.f))
			{
				currentResource = newLocation;
				// Ensure we're not exactly 0,0,0 because I designed this poorly
				currentResource.Z = !currentResource ? 0.1f : currentResource.Z;
				break;
			}
		}
	}
}

// Find the nearest resource. Uses Manhattan distance
// Manhattan distance of -1 indicates no resource was found
gv::Position TestWorldResourceLocator::FindNearestResource(const WorldResourceType type,
                                                           const gv::Position& location,
                                                           bool allowSameLocation,
                                                           float& manhattanToOut)
{
	gv::Position zeroPosition;
	if (ResourceListExists(type))
	{
		gv::Position closestResource;
		float closestResourceDistance = std::numeric_limits<float>::max();

		for (gv::Position& currentResource : *Resources[type])
		{
			float currentResourceDistance = location.ManhattanTo(currentResource);
			if (currentResourceDistance < closestResourceDistance &&
			    (allowSameLocation || currentResourceDistance > 0.f))
			{
				closestResourceDistance = currentResourceDistance;
				closestResource = currentResource;
			}
		}

		manhattanToOut = closestResourceDistance;
		return closestResource;
	}

	manhattanToOut = -1.f;
	return zeroPosition;
}