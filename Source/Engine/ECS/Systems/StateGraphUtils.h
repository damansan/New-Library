#pragma once
#include <string>
#include "Engine/Data/AnimatorGraphData.h"

namespace StateGraphUtils
{
	// State–¼‚©‚çid‚ð’T‚·
	int FindStateIdByName(const ParentStateGraph& parentGraph, const std::string& name);
}