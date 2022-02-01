#pragma once
#include <vector>
#include <string>
#include <string_view>
#include "stop.h"

struct Bus {
	struct Stop{
		size_t stop_id;
		std::string stop_name;
	};

	std::string name;
	size_t unique_stops_count;
	std::vector<Stop> stops;
	RouteType route_type;

	const size_t GetSize() const;
};


