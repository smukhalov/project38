#include "busmanager.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <unordered_set>
#include <utility>
#include <unordered_set>
#include "stringhelper.h"

BusManager::BusManager(): last_init_id(0){}

BusManager& BusManager::ReadSettings(const std::map<std::string, Json::Node>& node) {

	for(const auto& [node_name, value]: node) {
		if(node_name == "bus_wait_time"){
			settings.bus_wait_time = (value.IsDouble()) ? value.AsDouble() : value.AsInt();
		} else if(node_name == "bus_velocity") {
			//В метрах/минуту
			settings.bus_velocity = ((value.IsDouble()) ? value.AsDouble() : value.AsInt()) * 1000.0 / 60.0;
 		} else {
 			throw std::invalid_argument("BusManager::ReadSettings unsupported argument name " + node_name);
 		}
	}
	return *this;
}

BusManager& BusManager::ReadData(const std::vector<Json::Node>& node){
	std::vector<std::string> data_types;
	for(const auto& node_array: node){
		const auto& node_map = node_array.AsMap();
		for(const auto& [node_name, value]: node_map) {
			if(node_name == "type"){
				data_types.push_back(value.AsString());
				break;
			}
		}
	}

	auto it_data_types = data_types.begin();
	for(const auto& node_array: node){
		const auto& node_map = node_array.AsMap();

		if(*it_data_types == "Stop"){
			Stop stop;
			stop.id = stops.size();

			std::unordered_map<std::string, size_t> other_stops;

			for(const auto& [node_name, value]: node_map) {
				//std::string node_name = item_map.first;
				if(node_name == "name"){
					stop.name = value.AsString();
				} else if(node_name == "latitude"){
					stop.point.latitude = value.IsDouble() ? value.AsDouble() : value.AsInt();
				} else if(node_name == "longitude"){
					stop.point.longitude = value.IsDouble() ? value.AsDouble() : value.AsInt();
				} else if(node_name == "road_distances"){
					const std::map<std::string, Json::Node>& item_stops = value.AsMap();
					for(const auto& [stop_name, stop_distance]: item_stops){
						other_stops.emplace( std::make_pair(stop_name, stop_distance.AsInt()) );
					}
				} else if(node_name == "type"){
					continue; //
				} else {
					throw std::invalid_argument("BusManager::ReadData: Stop unsupported argument name " + node_name);
				}
			}

			for(const auto& [other_stop_name, other_stop_distance]: other_stops){
				stop_distances.emplace(std::make_pair(StopPair{stop.name, other_stop_name}, other_stop_distance));
			}

			stops.emplace(std::pair<std::string, Stop>{stop.name, stop});
		} else if(*it_data_types == "Bus"){
			Bus bus;
			for(const auto& [node_name, value]: node_map) {
				if(node_name == "name"){
					bus.name = value.AsString();
				} else if(node_name == "is_roundtrip"){
					bus.route_type = value.AsBool() ? RouteType::Round : RouteType::Line;
				} else if(node_name == "stops"){
					const std::vector<Json::Node>& item_stops = value.AsArray();
					std::unordered_set<std::string> unique_stops;

					size_t bus_stop_id = 0;
					for(const auto& item_stop: item_stops){
						bus.stops.push_back({bus_stop_id++, item_stop.AsString()});
						unique_stops.insert(item_stop.AsString());
					}
					bus.unique_stops_count = unique_stops.size();
				} else if(node_name == "type"){
					continue;
				} else {
					throw std::invalid_argument("BusManager::ReadData: Bus unsupported argument name " + node_name);
				}
			}

			buses.emplace(std::pair<std::string, Bus>{bus.name, bus});
			for(const auto& stop: bus.stops){
				stop_to_buses[stop.stop_name].insert(bus.name);
			}
		} else {
			throw std::invalid_argument("BusManager::ReadData: unsupported data type " + *it_data_types);
		}

		it_data_types++;
	}

	return *this;
}

BusManager& BusManager::ReadRequest(const std::vector<Json::Node>& node){
	for(const auto& node_array: node){
		const auto& node_map = node_array.AsMap();
		std::unique_ptr<Command> command;

		for(const auto& [node_name, value]: node_map) {
			if(node_name == "type"){
				const std::string type = value.AsString();
				if(type == "Bus"){
					command = std::make_unique<BusCommand>();
				} else if(type == "Stop"){
					command = std::make_unique<StopCommand>();
				} else if(type == "Route"){
					command = std::make_unique<RouteCommand>();
				}else {
					throw std::invalid_argument("BusManager::ReadRequest: unsupported command type " + type);
				}

				commands.push_back(move(command));
				break;
			}
		}
	}

	auto it_command_types = commands.begin();
	for(const auto& node_array: node){
		const auto& node_map = node_array.AsMap();

		for(const auto& [node_name, value]: node_map) {
			if(node_name == "id"){
				(*it_command_types)->id = value.AsInt();
				continue;
			} else if(node_name == "type"){
				continue;
			}

			if((*it_command_types)->GetType() == CommandType::Bus){
				if(node_name == "name"){
					((BusCommand*)(it_command_types->get()))->name = value.AsString();
				}
			} else if((*it_command_types)->GetType() == CommandType::Stop){
				if(node_name == "name"){
					((StopCommand*)(it_command_types->get()))->name = value.AsString();
				}
			} else if((*it_command_types)->GetType() == CommandType::Route){
				if(node_name == "from"){
					((RouteCommand*)(it_command_types->get()))->stop_from = value.AsString();
				} else if(node_name == "to") {
					((RouteCommand*)(it_command_types->get()))->stop_to = value.AsString();
				} else {
					throw std::invalid_argument("BusManager::ReadRequest: RouteCommand unsupported key: " + node_name);
				}
			} else {
				throw std::invalid_argument("BusManager::ReadRequest: unsupported command type");
			}
		}

		it_command_types++;
	}

	return *this;
}

BusManager& BusManager::Read(std::istream& in){
	Json::Document doc = Json::Load(in);
	const Json::Node& node_root = doc.GetRoot();
	const auto& root_map = node_root.AsMap();

	for(const auto& [node_name, value]: root_map){
		//const std::string node_name = node.first;
		if(node_name == "base_requests"){
			ReadData(value.AsArray());
		} else if(node_name == "stat_requests"){
			ReadRequest(value.AsArray());
		} else if(node_name == "routing_settings"){
			ReadSettings(value.AsMap());
		} else {
			throw std::invalid_argument("Incorrect root node_name - " + node_name);
		}
	}

	//Заполним ребра на основе первичных данных
	for(const auto& [_, bus]: buses){
		if(bus.route_type == RouteType::Line){
			FillEdgesLine(bus);
		} else {
			FillEdgesRound(bus);
		}
	}

	return *this;
}

void BusManager::WriteResponse(std::ostream& out) const {
	size_t vertex_count = last_init_id;
	Graph::DirectedWeightedGraph<double> graph(vertex_count);

	std::cout << "vertex_count - " << vertex_count << std::endl;
	std::cout << "edges_count - " << edges.size() << std::endl;

	if(logging){
		std::cout << "edges list\n";
	}
	for(const auto& [from, to, distance, route_item_type]: edges){
		if(logging){
			std::cout << "from - " << from << ", to - " << to << "; distance - " << distance << "; route_item_type - "
					<< (route_item_type == RouteItemType::Bus ? "Bus" : "Wait") << std::endl;
		}
		graph.AddEdge({from, to, distance, route_item_type});
	}

	if(logging){
		std::cout << std::endl;
		std::cout << "vertex_to_bus_stop count - " << vertex_to_bus_stop.size() << "\n";
		for(const auto& [vertex_id, bus_to_stop_set]: vertex_to_bus_stop){
			std::cout << "vertex_id - " << vertex_id << std::endl;
			for(const auto& bus_to_stop: bus_to_stop_set){
				std::cout << "\t\tbus_to_stop.bus_name - " << bus_to_stop.bus_name
									<< "; bus_to_stop_name - " << bus_to_stop.stop_name << std::endl;
			}
		}

		std::cout << std::endl;
		std::cout << "stop_to_bus_vertex count - " << stop_to_bus_vertex.size() << "\n";
		for(const auto& [stop_name, bus_vertex_set]: stop_to_bus_vertex){
			std::cout << "stop_name - " << stop_name  << std::endl;
			for(const auto& bus_vertex: bus_vertex_set){
				std::cout << "\t\t" << "vertex_id - " << bus_vertex.vertex_id << "; bus_name - " << bus_vertex.bus_name << std::endl;
			}
		}

		std::cout << std::endl;
		std::cout << "bus_stop_to_vertex count - " << bus_stop_to_vertex.size() << "\n";
		for(const auto& [bus_stop, vertex_id]: bus_stop_to_vertex){
			std::cout << "vertex_id - " << vertex_id << ", bus_name - " << bus_stop.bus_name
					<< "; stop_id - " << bus_stop.stop_id
					<< "; stop_name - " << bus_stop.stop_name << std::endl;
		}
	}

	Graph::Router<double> router(graph);
	//return;

	//out << std::fixed << std::setprecision(6) << "[\n";
	out << "[\n";

	size_t count = commands.size();
	size_t n = 0;
	for(const auto& command: commands){
		out << "\t{\n";
		out << "\t\t\"request_id\": " << command->id << ",\n";
		if(command->GetType() == CommandType::Bus){
			if(const auto it = buses.find(((BusCommand*)(command.get()))->name); it == buses.end()){
				out << "\t\t\"error_message\": \"not found\"\n";
			} else {
				size_t distance_by_stops = GetDistanceByStops(it->second);
				out << "\t\t\"stop_count\": " << it->second.GetSize() << ",\n";
				out << "\t\t\"unique_stop_count\": " << it->second.unique_stops_count << ",\n";
				out << "\t\t\"route_length\": " << distance_by_stops << ",\n";
				out << "\t\t\"curvature\": " << distance_by_stops / GetDistanceByGeo(it->second) << "\n";
			}
		} else if(command->GetType() == CommandType::Stop){
			if(const auto it = stop_to_buses.find(((StopCommand*)(command.get()))->name); it == stop_to_buses.end()){
				out << "\t\t\"error_message\": \"not found\"\n";
			} else if(it->second.size() == 0) {
				out << "\t\t\"buses\": []\n";
			} else {
				size_t bus_count = it->second.size();
				size_t b = 0;
				out << "\t\t\"buses\": [\n";
				for(const auto& item: it->second){
					out << "\t\t\t\"" << item << "\"";
					if(++b < bus_count){
						out << ",\n";
					}
				}
				out << "\n\t\t]\n";
			}
		} else if(command->GetType() == CommandType::Route){
			RouteCommand rc = *(RouteCommand*)(command.get());
			if(rc.stop_from == rc.stop_to){
				out << "\t\t\"total_time\": 0,\n\t\t\"items\": []\n";
			} else {
				Route route = BuildBestRoute(rc, graph, router);

				if(route.items.size() == 0){
					out << "\t\t\"error_message\": \"not found\"\n";
				} else {
					out << "\t\t\"total_time\": " << route.total_time << ",\n";

					size_t items_count = route.items.size();
					size_t n = 0;
					out << "\t\t\"items\": [\n";
					for(const auto& item: route.items){
						item->Print(out);
						if(++n < items_count){
							out << ",\n";
						}
					}
					out << "\n\t\t]\n";
				}
			}
		}
		out << "\t}";
		if(++n < count) {
			out << ",\n";
		}
	}
	out << "\n]";
}

Route BusManager::BuildBestRoute(const RouteCommand& command,
			const Graph::DirectedWeightedGraph<double>& graph,
			Graph::Router<double>& router) const {

	Route route;
	route.total_time = -1.0;

	std::vector<Graph::VertexId> vertex_from_list;
	if(auto it = stop_to_buses.find(command.stop_from); it != stop_to_buses.end()){
		for(const std::string& bus_name: it->second){
			const Bus& bus = buses.at(bus_name);
			const std::vector<Bus::Stop>& stops = bus.stops;

			//В круговых маршрутах допускается несколько остановок с одинаковым названием. Находим их все
			auto it = stops.begin();
			while(true){
				it = std::find_if(it, stops.end(), [&](const Bus::Stop& stop){return stop.stop_name == command.stop_from;});
				if(it == stops.end()){
					break;;
				}

				if(auto it_bus_stop = bus_stop_to_vertex.find({bus_name, it->stop_id}); it_bus_stop != bus_stop_to_vertex.end()){
					vertex_from_list.push_back(it_bus_stop->second);
				}

				it = it + 1;
			}
		}
	}

	if(vertex_from_list.size() == 0){
		return route;
	}

	std::vector<Graph::VertexId> vertex_to_list;
	if(auto it = stop_to_buses.find(command.stop_to); it != stop_to_buses.end()){
		for(const std::string& bus_name: it->second){
			const Bus& bus = buses.at(bus_name);
			const std::vector<Bus::Stop>& stops = bus.stops;

			auto it = stops.begin();
			while(true){
				it = std::find_if(it, stops.end(), [&](const Bus::Stop& stop){return stop.stop_name == command.stop_to;});
				if(it == stops.end()){
					break;;
				}

				if(auto it_bus_stop = bus_stop_to_vertex.find({bus_name, it->stop_id}); it_bus_stop != bus_stop_to_vertex.end()){
					vertex_to_list.push_back(it_bus_stop->second);
				}

				it = it + 1;
			}
		}
	}

	if(vertex_to_list.size() == 0){
		return route;
	}

	std::vector<Graph::EdgeId> route_edges;
	for(const Graph::VertexId vertex_from: vertex_from_list){
		for(const Graph::VertexId vertex_to: vertex_to_list){
			auto route_info = router.BuildRoute(vertex_from, vertex_to);
			if(!route_info || route_info->edge_count < 1){
				continue;
			} else if(route.total_time < 0 || route_info->weight < route.total_time) {
				route_edges.clear();

				route.total_time = route_info->weight;
				for(size_t i = 0; i < route_info->edge_count; i++) {
					route_edges.push_back(router.GetRouteEdge(route_info->id, i));
				}

				router.ReleaseRoute(route_info->id);
			}
		}
	}

	if(route_edges.size() == 0){
		return route;
	}

	route.total_time += settings.bus_wait_time;

	RouteItemWait rw;
	rw.stop_name = command.stop_from;
	rw.bus_wait_time = settings.bus_wait_time;

	route.items.push_back(std::make_shared<RouteItemWait>(rw));

	auto it_route_edges_begin = route_edges.begin();
	auto it_route_edges_end = route_edges.end();

	while(true) {
		auto it_route_edges_end_current = std::find_if(it_route_edges_begin, it_route_edges_end, [&](const Graph::EdgeId& edge_id) {
			const Graph::Edge<double>& edge = graph.GetEdge(edge_id);
			return edge.route_item_type == RouteItemType::Wait;
		});

		const Graph::Edge<double>& edge = graph.GetEdge(*it_route_edges_begin);
		const auto& bus_stop_from_set = vertex_to_bus_stop.at(edge.from);
		const auto& bus_stop_to_set = vertex_to_bus_stop.at(edge.to);

		std::vector<std::string> bus_range_list;
		std::transform(bus_stop_from_set.begin(), bus_stop_from_set.end(), std::back_inserter(bus_range_list),
				[](const BusStop& bus_stop){
				return bus_stop.bus_name;
		});

		uint32_t span_count = std::distance(it_route_edges_begin, it_route_edges_end_current);
		double bus_move_time = edge.weight;

		std::string stop_to = bus_stop_to_set.begin()->stop_name;
		for(auto it = ++it_route_edges_begin; it != it_route_edges_end_current; ++it ){
			const Graph::Edge<double>& edge = graph.GetEdge(*it);
			bus_move_time += edge.weight;

			const auto& bus_stop_from_set = vertex_to_bus_stop.at(edge.from);
			const auto& bus_stop_to_set = vertex_to_bus_stop.at(edge.to);

			stop_to = bus_stop_to_set.begin()->stop_name;

			std::vector<std::string> buses_for_delete;
			for(const std::string& bus_name: bus_range_list){
				bool found = false;
				for(const BusStop& bus_stop: bus_stop_from_set){
					if(bus_name == bus_stop.bus_name){
						found = true;
						break;
					}
				}

				if(!found){
					buses_for_delete.push_back(bus_name);
				}
			}

			for(const std::string bus_for_felete: buses_for_delete){
				auto p = std::remove(bus_range_list.begin(), bus_range_list.end(), bus_for_felete);
				bus_range_list.erase(p, bus_range_list.end());
			}
		}

		assert(bus_range_list.size() > 0);

		RouteItemBus rb;
		rb.bus_number = bus_range_list[0];
		rb.span_count = span_count;
		rb.bus_move_time = bus_move_time;
		route.items.push_back(std::make_shared<RouteItemBus>(rb));

		if(it_route_edges_end_current == it_route_edges_end){
			break;
		}

		RouteItemWait rw;
		rw.stop_name = stop_to;
		rw.bus_wait_time = settings.bus_wait_time;

		route.items.push_back(std::make_shared<RouteItemWait>(rw));

		it_route_edges_begin = it_route_edges_end_current + 1;
	}

	return route;
}

double BusManager::GetDistance(std::vector<std::string>::const_iterator it) const {
	double bus_time;

	if(auto it_distance_b = stop_distances.find(StopPair{*it, *(it+1)}); it_distance_b != stop_distances.end()) {
		bus_time = it_distance_b->second / settings.bus_velocity;
	} else if(auto it_distance_e = stop_distances.find(StopPair{*(it+1), *it});	it_distance_e != stop_distances.end()) {
		bus_time = it_distance_e->second / settings.bus_velocity;
	} else {
		throw std::invalid_argument("Not found distance " + (*it) + " and " + (*(it+1)));
	}

	return bus_time;
}

size_t BusManager::GetDistanceByStops(const Bus& bus) const {
	if(bus.stops.size() < 2){
		return 0;
	}

	size_t result = GetDistanceByStops(bus, true);
	if(bus.route_type == RouteType::Line){
		result += GetDistanceByStops(bus, false);
	}

	return result;
}

size_t BusManager::GetDistanceByStops(const Bus& bus, bool forward) const {
	size_t result = 0;
	size_t stop_count =	bus.stops.size();

	auto it_end = stop_distances.end();
	size_t i = 0;
	do{
		size_t first_stop = forward ? i : i + 1;
		size_t second_stop = forward ? i + 1 : i;

		const Bus::Stop& bus_stop_1 = bus.stops[first_stop];
		const Bus::Stop& bus_stop_2 = bus.stops[second_stop];

		std::string stop_name_1 = bus_stop_1.stop_name;
		std::string stop_name_2 = bus_stop_2.stop_name;

		auto it = stop_distances.find({stop_name_1, stop_name_2});
		if(it == it_end){
			it = stop_distances.find({stop_name_2, stop_name_1});
			if(it == it_end){
				throw std::invalid_argument("[" + stop_name_1 + ", " + stop_name_2 + "] not found");
			}
		}

		result += it->second;
		i++;
	} while(i < stop_count - 1);

	return result;
}

double BusManager::GetDistanceByGeo(const Bus& bus) const {
	double result = 0.0;
    size_t stop_count =	bus.stops.size();
	if(bus.stops.size() < 2){
		return 0.0;
	}

	const std::vector<Bus::Stop>& stops_for_bus = bus.stops;
	auto it_end = stops.end();

	for(size_t i = 0; i < stop_count - 1; i++){
		const Bus::Stop bus_stop_1 = stops_for_bus[i];
		const Bus::Stop bus_stop_2 = stops_for_bus[i+1];

		const std::string& stop_name_1 = bus_stop_1.stop_name;
		const std::string& stop_name_2 = bus_stop_2.stop_name;

		auto it1 = stops.find(stop_name_1);
		if(it1 == it_end){
			throw std::invalid_argument(stop_name_1 + " not found");
		}

		auto it2 = stops.find(stop_name_2);
		if(it2 == it_end){
			throw std::invalid_argument(stop_name_2 + " not found");
		}

		result += Distance(it1->second.point, it2->second.point).Length();
	}

	if(bus.route_type == RouteType::Line){
		result *= 2;
	}

	return result;
}

void BusManager::FillEdgesLine(const Bus& bus){
	assert(bus.route_type == RouteType::Line);

	const std::vector<Bus::Stop>& stops_for_bus = bus.stops;
	size_t stop_count = stops_for_bus.size();

	auto it_end = stop_distances.end();
	for(size_t i = 0; i < stop_count-1; ++i ){
		const Bus::Stop bus_stop_1 = stops_for_bus[i];
		const Bus::Stop bus_stop_2 = stops_for_bus[i+1];

		const std::string stop_name_1 = bus_stop_1.stop_name;
		const size_t stop_id_1 = bus_stop_1.stop_id;

		const std::string stop_name_2 = bus_stop_2.stop_name;
		const size_t stop_id_2 = bus_stop_2.stop_id;

		auto it_stop_1_2 = stop_distances.find({stop_name_1, stop_name_2});
		auto it_stop_2_1 = stop_distances.find({stop_name_2, stop_name_1});

		if(it_stop_1_2 == it_end && it_stop_2_1 == it_end){
			throw std::invalid_argument("FillEdgesLine. distance [" + stop_name_1 + ", " + stop_name_2 + "] not found");
		}

		double distance_1_2 = -1.0;
		double distance_2_1 = -1.0;

		if(it_stop_1_2 != it_end){
			distance_1_2 = it_stop_1_2->second / settings.bus_velocity;
		}

		if(it_stop_2_1 != it_end){
			distance_2_1 = it_stop_2_1->second / settings.bus_velocity;
			if(distance_1_2 < 0){
				distance_1_2 = distance_2_1;
			}
		} else {
			distance_2_1 = distance_1_2;
		}

		assert(distance_1_2 >= 0 && distance_2_1 >= 0);

		size_t vertex_1 = -1;
		if(auto it = bus_stop_to_vertex.find({bus.name, stop_id_1}); it != bus_stop_to_vertex.end()){
			vertex_1 = it->second;
		} else {
			vertex_1 = last_init_id++;
		}

		size_t vertex_2 = -1;
		if(auto it = bus_stop_to_vertex.find({bus.name, stop_id_2}); it != bus_stop_to_vertex.end()){
			vertex_2 = it->second;
		} else {
			vertex_2 = last_init_id++;
		}

		assert(vertex_1 >= 0 && vertex_2 >= 0);

		vertex_to_bus_stop[vertex_1].insert({bus.name, stop_id_1, stop_name_1});
		vertex_to_bus_stop[vertex_2].insert({bus.name, stop_id_2, stop_name_2});

		AddEdge({vertex_1, vertex_2, distance_1_2, RouteItemType::Bus});
		AddEdge({vertex_2, vertex_1, distance_2_1, RouteItemType::Bus});

		if(auto it = stop_to_bus_vertex.find(stop_name_1); it != stop_to_bus_vertex.end()){
			for(const BusVertex bus_vertex: it->second){
				if(bus_vertex.bus_name == bus.name){
					continue;
				}

				AddEdge({vertex_1, bus_vertex.vertex_id, settings.bus_wait_time, RouteItemType::Wait});
				AddEdge({bus_vertex.vertex_id, vertex_1, settings.bus_wait_time, RouteItemType::Wait});
			}
		}

		if(auto it = stop_to_bus_vertex.find(stop_name_2); it != stop_to_bus_vertex.end()){
			for(const BusVertex bus_vertex: it->second){
				if(bus_vertex.bus_name == bus.name){
					continue;
				}

				AddEdge({vertex_2, bus_vertex.vertex_id, settings.bus_wait_time, RouteItemType::Wait});
				AddEdge({bus_vertex.vertex_id, vertex_2, settings.bus_wait_time, RouteItemType::Wait});
			}
		}

		stop_to_bus_vertex[stop_name_1].insert({bus.name, vertex_1});
		stop_to_bus_vertex[stop_name_2].insert({bus.name, vertex_2});

		bus_stop_to_vertex.insert({{bus.name, stop_id_1, stop_name_1}, vertex_1});
		bus_stop_to_vertex.insert({{bus.name, stop_id_2, stop_name_2}, vertex_2});
	}
}

void BusManager::FillEdgesRound(const Bus& bus){
	assert(bus.route_type == RouteType::Round);

	const std::vector<Bus::Stop>& stops_for_bus = bus.stops;
	size_t stop_count = stops_for_bus.size();

	auto it_end = stop_distances.end();
	for(size_t i = 0; i < stop_count-2; ++i ){
		const Bus::Stop bus_stop_1 = stops_for_bus[i];
		const Bus::Stop bus_stop_2 = stops_for_bus[i+1];

		const std::string stop_name_1 = bus_stop_1.stop_name;
		const size_t stop_id_1 = bus_stop_1.stop_id;

		const std::string stop_name_2 = bus_stop_2.stop_name;
		const size_t stop_id_2 = bus_stop_2.stop_id;

		auto it_stop_1_2 = stop_distances.find({stop_name_1, stop_name_2});
		auto it_stop_2_1 = stop_distances.find({stop_name_2, stop_name_1});

		if(it_stop_1_2 == it_end && it_stop_2_1 == it_end){
			throw std::invalid_argument("FillEdgesLine. distance [" + stop_name_1 + ", " + stop_name_2 + "] not found");
		}

		double distance_1_2 = -1.0;

		if(it_stop_1_2 != it_end){
			distance_1_2 = it_stop_1_2->second / settings.bus_velocity;
		} else {
			distance_1_2 = it_stop_2_1->second / settings.bus_velocity;
		}

		assert(distance_1_2 >= 0);

		size_t vertex_1 = -1;
		if(auto it = bus_stop_to_vertex.find({ bus.name, stop_id_1 }); it != bus_stop_to_vertex.end()){
			vertex_1 = it->second;
		} else {
			vertex_1 = last_init_id++;
		}

		size_t vertex_2 = -1;
		if(auto it = bus_stop_to_vertex.find({ bus.name, stop_id_2 }); it != bus_stop_to_vertex.end()){
			vertex_2 = it->second;
		} else {
			vertex_2 = last_init_id++;
		}

		assert(vertex_1 >= 0 && vertex_2 >= 0);

		vertex_to_bus_stop[vertex_1].insert({bus.name, stop_id_1, stop_name_1});
		vertex_to_bus_stop[vertex_2].insert({bus.name, stop_id_2, stop_name_2});

		AddEdge({vertex_1, vertex_2, distance_1_2, RouteItemType::Bus});

		if(auto it = stop_to_bus_vertex.find(stop_name_1); it != stop_to_bus_vertex.end()){
			for(const BusVertex bus_vertex: it->second){
				if(bus_vertex.bus_name == bus.name && vertex_1 == bus_vertex.vertex_id){
					continue;
				}

				AddEdge({vertex_1, bus_vertex.vertex_id, settings.bus_wait_time, RouteItemType::Wait});
				AddEdge({bus_vertex.vertex_id, vertex_1, settings.bus_wait_time, RouteItemType::Wait});
			}
		}

		if(auto it = stop_to_bus_vertex.find(stop_name_2); it != stop_to_bus_vertex.end()){
			for(const BusVertex bus_vertex: it->second){
				if(bus_vertex.bus_name == bus.name && vertex_2 == bus_vertex.vertex_id){
					continue;
				}

				AddEdge({vertex_2, bus_vertex.vertex_id, settings.bus_wait_time, RouteItemType::Wait});
				AddEdge({bus_vertex.vertex_id, vertex_2, settings.bus_wait_time, RouteItemType::Wait});
			}
		}

		stop_to_bus_vertex[stop_name_1].insert({bus.name, vertex_1});
		stop_to_bus_vertex[stop_name_2].insert({bus.name, vertex_2});

		bus_stop_to_vertex.insert({{bus.name, stop_id_1, stop_name_1}, vertex_1});
		bus_stop_to_vertex.insert({{bus.name, stop_id_2, stop_name_2}, vertex_2});
	}

	//Остановка stops_for_bus.size()-2  -- stops_for_bus.size()-1
	{
		const std::string& stop_name_1 = stops_for_bus[stop_count-2].stop_name;
		const size_t& stop_id_1 = stops_for_bus[stop_count-2].stop_id;
		const size_t& vertex_1 = bus_stop_to_vertex.at({bus.name, stop_id_1});

		const std::string& stop_name_2 = stops_for_bus[stop_count-1].stop_name;
		const size_t& stop_id_2 = stops_for_bus[stop_count-1].stop_id;
		size_t vertex_2 = last_init_id++;

		AddEdge({vertex_1, vertex_2,
			stop_distances.at({stop_name_1, stop_name_2})/settings.bus_velocity, RouteItemType::Bus});

		vertex_to_bus_stop[vertex_1].insert({bus.name, stop_id_1, stop_name_1});
		vertex_to_bus_stop[vertex_2].insert({bus.name, stop_id_2, stop_name_2});

		stop_to_bus_vertex[stop_name_1].insert({bus.name, vertex_1});
		stop_to_bus_vertex[stop_name_2].insert({bus.name, vertex_2});

		bus_stop_to_vertex.insert({{bus.name, stop_id_1, stop_name_1}, vertex_1});
		bus_stop_to_vertex.insert({{bus.name, stop_id_2, stop_name_2}, vertex_2});
	}

	{
		const std::string& stop_name_1 = stops_for_bus[stop_count-1].stop_name;
		const size_t& stop_id_1 = stops_for_bus[stop_count-1].stop_id;
		const size_t& vertex_1 = bus_stop_to_vertex.at({bus.name, stop_id_1});

		const std::string& stop_name_2 = stop_name_1;
		const size_t& stop_id_2 = stops_for_bus[0].stop_id;
		size_t vertex_2 = bus_stop_to_vertex.at({bus.name, stop_id_2});

		AddEdge({vertex_1, vertex_2, settings.bus_wait_time, RouteItemType::Wait});

		vertex_to_bus_stop[vertex_1].insert({bus.name, stop_id_1, stop_name_1});
		vertex_to_bus_stop[vertex_2].insert({bus.name, stop_id_2, stop_name_2});

		stop_to_bus_vertex[stop_name_1].insert({bus.name, vertex_1});
		stop_to_bus_vertex[stop_name_2].insert({bus.name, vertex_2});

		bus_stop_to_vertex.insert({{bus.name, stop_id_1, stop_name_1}, vertex_1});
		bus_stop_to_vertex.insert({{bus.name, stop_id_2, stop_name_2}, vertex_2});
	}
}

void BusManager::AddEdge(const Edge& edge){
	auto it = edges.find({edge.from, edge.to});
	if(it == edges.end()){
		edges.insert({edge.from, edge.to, edge.distance, edge.route_item_type});
		return;
	}

	if(it->distance < edge.distance){
		return;
	}

	edges.insert({edge.from, edge.to, edge.distance, edge.route_item_type});
}
