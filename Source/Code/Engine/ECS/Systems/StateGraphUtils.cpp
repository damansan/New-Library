#include "StateGraphUtils.h"

namespace StateGraphUtils
{
	int FindStateIdByName(const ParentStateGraph& parentGraph, const std::string& name)
	{
		for (const auto& s : parentGraph.states)
		{
			if (s.name == name) return s.id;
		}
		return -1;
	}
}