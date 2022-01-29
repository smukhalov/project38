#pragma once
#include <vector>
#include <string>
#include <string_view>
#include "stop.h"

struct Bus {
	std::string name;
	std::vector<std::string> stops;
	RouteType route_type;

	const size_t GetSize() const;
	const size_t GetUniqueSize() const;
};


