#pragma once

#include <map>
#include <vector>

#include "world/Position.hpp"

enum WorldResourceType
{
	None = 0,
	Agent = 1,
	BusStop = 2
};

// This is for testing only and will not be used in the final game
class TestWorldResourceLocator
{
private:
	typedef std::vector<gv::Position> ResourceList;
	typedef std::map<WorldResourceType, ResourceList*> ResourceMap;

	ResourceMap Resources;

	bool ResourceListExists(const WorldResourceType type) const;

public:
	TestWorldResourceLocator() = default;
	~TestWorldResourceLocator();

	bool ResourceExistsInWorld(const WorldResourceType type);

	void AddResource(const WorldResourceType type, const gv::Position& location);
	void RemoveResource(const WorldResourceType type, const gv::Position& location);
	void MoveResource(const WorldResourceType type, const gv::Position& oldLocation,
	                  const gv::Position& newLocation);

	// Find the nearest resource. Uses Manhattan distance
	// Manhattan distance of -1 indicates no resource was found
	gv::Position FindNearestResource(const WorldResourceType type, const gv::Position& location,
	                                 float& manhattanToOut);
};