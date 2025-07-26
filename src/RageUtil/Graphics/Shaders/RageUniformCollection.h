#ifndef RAGE_UNIFORM_COLLECTION_H
#define RAGE_UNIFORM_COLLECTION_H

#include "RageUniform.h"

struct RageUniformCollection
{
	std::vector<RageUniform<float>> floatData;
	std::vector<RageUniform<int>> intData;
	std::vector<RageUniform<int>> boolData;
};

#endif
