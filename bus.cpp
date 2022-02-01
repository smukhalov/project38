#include "bus.h"

const size_t Bus::GetSize() const {
	if(route_type == RouteType::Round){
		return stops.size();
	}

	return 2*stops.size() - 1;
}




