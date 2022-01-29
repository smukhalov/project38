#include <unordered_set>
#include <algorithm>
#include <iterator>

#include "bus.h"
#include "stringhelper.h"

/*
Bus::Bus(std::string_view name_): name(name_), route_type(RouteType::Round){
}

void Bus::Read(std::istream& in) {
	std::string line;
	in >> std::ws;
	getline(in, line);

	char delimiter = '>';
	std::string_view sv(line);
	if(size_t pos1 = sv.find('>'), pos2 = sv.find('-');
		pos1 == std::string::npos && pos2 == std::string::npos){
		stops.push_back(Stop(Trim(sv)));
		return;
	} else if(pos1 == std::string::npos){
		delimiter = '-';
		route_type = RouteType::Line;
	}

	while(true){
		size_t pos = sv.find(delimiter);
		if(pos == std::string::npos){
			stops.push_back(Stop(Trim(sv)));
			break;
		}

		std::string_view part = sv.substr(0, pos);
		stops.push_back(Stop(Trim(part)));
		sv.remove_prefix(pos + 1);
	}
}*/

const size_t Bus::GetSize() const {
	if(route_type == RouteType::Round){
		return stops.size();
	}

	return 2*stops.size() - 1;
}

const size_t Bus::GetUniqueSize() const {
	std::unordered_set<std::string> unique_stops;
	std::copy(stops.begin(), stops.end(), std::inserter(unique_stops, unique_stops.end()));
	//std::transform(stops.begin(), stops.end(),
	//		std::inserter(unique_stops, unique_stops.end()),
	//		[](const auto& item) -> std::string { return  item.name;});

	return unique_stops.size();
}



