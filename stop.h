#pragma once
#include <string>
#include <string_view>
#include "distance.h"

struct Stop{
	size_t id;
	std::string name;
	Point point;
};
