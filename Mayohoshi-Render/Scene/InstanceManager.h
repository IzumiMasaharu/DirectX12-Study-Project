#pragma once
#include <vector>
#include "ShaderResource.h"

class InstancePool
{
public:
	InstancePool() = default;
	~InstancePool() = default;


public:
	std::vector<InstanceData> instanceDatas;
};

class InstanceManager
{
public:
	InstanceManager() = default;
	~InstanceManager() = default;

private:

	InstancePool instancePool;
};

