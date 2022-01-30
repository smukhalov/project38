#include "busmanager.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <utility>
#include <unordered_set>
#include "stringhelper.h"

BusManager::BusManager(): last_init_id(0)
{}

BusManager& BusManager::ReadSettings(const std::map<std::string, Json::Node>& node) {

	for(const auto& [node_name, value]: node) {
		//std::string node_name = item_map.first;
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
			//std::string node_name = item_map.first;
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
					//throw std::invalid_argument("BusManager::ReadData: Stop - " + log.str());
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
				//std::string node_name = item_map.first;
				if(node_name == "name"){
					bus.name = value.AsString();
				} else if(node_name == "is_roundtrip"){
					bus.route_type = value.AsBool() ? RouteType::Round : RouteType::Line;
				} else if(node_name == "stops"){
					const std::vector<Json::Node>& item_stops = value.AsArray();
					for(const auto& item_stop: item_stops){
						bus.stops.push_back(item_stop.AsString());
					}
				} else if(node_name == "type"){
					continue;
				} else {
					//throw std::invalid_argument("BusManager::ReadData: Bus - " + log.str());
					throw std::invalid_argument("BusManager::ReadData: Bus unsupported argument name " + node_name);
				}
			}

			buses.emplace(std::pair<std::string, Bus>{bus.name, bus});
			for(const std::string& stop_name: bus.stops){
				stop_to_buses[stop_name].insert(bus.name);
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
			//std::string node_name = item_map.first;
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
			//std::string node_name = item_map.first;
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
		FillEdges(bus);
	}

	//Создаем справочники stop_id_to_buses, stop_id_to_stops
	/*for(const auto& [stop_name, buses_for_stop]: stop_to_buses){
		Graph::VertexId vertex_id = stops.at(stop_name).id;
		std::set<std::string> buses_copy = buses_for_stop;

		stop_id_to_buses.emplace(std::make_pair(vertex_id, buses_copy));
		stop_id_to_stops.emplace(std::make_pair(vertex_id, stop_name));
	}

	//last_init_id = stops.size();

	*/

	//Обходим маршруты на предмет пересадок, когда у одной остановки более одного маршрута
	/*for(const auto& [stop_name, buses_for_stop]: stop_to_buses){
		if(buses_for_stop.size() > 1){
			std::vector<size_t> v;
			const Stop& stop = stops.at(stop_name);

			for(auto it = buses_for_stop.begin(); it != buses_for_stop.end(); it++){
				const Bus& bus = buses.at(*it);

				if(bus.route_type == RouteType::Round){
					const std::vector<size_t>& virt_ids = stop_to_transfer_for_round.at(stop_name);
					std::copy(virt_ids.begin(), virt_ids.end(), std::back_inserter(v));
					continue;
				}

				stop_id_to_stops.emplace(std::make_pair(last_init_id, stop_name));
				stop_id_to_buses[last_init_id].emplace(*it);

				std::vector<Graph::Edge<double>>& edges_for_bus = bus_to_edges[*it];
				for(auto& [from, to, _]: edges_for_bus){
					if(from == stop.id){
						from = last_init_id;
					}

					if(to == stop.id){
						to = last_init_id;
					}
				}
				v.push_back(last_init_id++);
			}

			if(auto it = stop_to_transfer.find(stop_name); it != stop_to_transfer.end()){
				std::copy(v.begin(), v.end(), std::inserter(it->second, it->second.end()));
			} else {
				stop_to_transfer.emplace(std::make_pair(stop_name, v));
			}
		}
	}*/

	/*std::unordered_set<StopIdPair, StopIdPairHasher> pair_for_remove;
	for(const auto& [bus_number, bus]: buses){
		const std::vector<std::string>& stops = bus.stops;
		//size_t count = stops.size();
		for(auto it = stops.begin(); it < stops.end() - 3; it++){
			auto range = {it, it + 3};
			for(const auto& [other_bus_number, other_bus]: buses){
				const std::vector<std::string>& other_stops = other_bus.stops;

				for(auto other_it = other_stops.begin(); other_it < other_stops.end() - 3; other_it++){
					if((*it == *other_it) && (*(it+1) == *(other_it+1)) && (*(it+1) == *(other_it+1))){
						break;
					}
				}
			}
		}
	}*/

	/*for(const auto& [stop_name, v]: stop_to_transfer){
		size_t count = v.size();
		//std::cout << "stop_name = " << stop_name << "; count = " << count << "\n";
		for(size_t i = 0; i < count; i++){
			for(size_t j = 0; j < count; j++){
				if(i == j){
					continue;
				}

				edges.push_back({v[i], v[j], double(settings.bus_wait_time)});
			}
		}
	}

	std::copy(stop_to_transfer_for_round.begin(), stop_to_transfer_for_round.end(), std::inserter(stop_to_transfer, stop_to_transfer.end()));
*/
	/*if(logging){
		std::cout << "stops\n";
		for(const auto& item: stops){
			std::cout << "[" << item.second.id << ", " << item.first << "]\n";
		}
		std::cout << "\n";

		std::cout << "stop_id_to_stops\n";
		for(const auto& item: stop_id_to_stops){
			std::cout << "[" << item.first << ", " << item.second << "]\n";
		}
		std::cout << "\n";

		std::cout << "stop_to_buses\n";
		for(const auto& [stop_name, buses]: stop_to_buses){
			std::cout << "[" << stop_name << ", ";

			for(const std::string& bus_number: buses){
				std::cout << bus_number << " ";
			}

			std::cout <<  "]\n";
		}
		std::cout << "\n";

		std::cout << "buses\n";
		for(const auto& [bus_number, bus]: buses){
			std::cout << "[" << bus_number << (bus.route_type == RouteType::Line ? " (Line) " : " (Round) ") << ", ";

			for(const auto& stop_name: bus.stops){
				std::cout << stop_name << " >> ";
			}

			std::cout <<  "]\n";
		}
		std::cout << "\n";

		std::cout << "bus_to_edges\n";
		for(const auto& [bus_number, e]: bus_to_edges){
			std::cout << "[" << bus_number << "\n";

			for(const auto& [from, to, distance]: e){
				std::cout << from << "  ---  " << to << " --- " <<  distance << "\n";
			}

			std::cout <<  "]\n";
		}
		std::cout << "\n";

		std::cout << "edges\n";
		for(const auto& [from, to, distance]: edges){
			std::cout << from << "  ---  " << to << " --- " <<  distance << "\n";
		}
		std::cout << "\n";
	}*/
	return *this;
}

void BusManager::WriteResponse(std::ostream& out) const {
	throw std::invalid_argument("BusManager::WriteResponse not inpemented");

	/*size_t vertex_count = stop_id_to_stops.size();
	std::cout << "stop_id_to_stops\n";
	for(const auto& [key, value]: stop_id_to_stops){
		std::cout << "VertexId - " << key << ", stop_name - " << value << std::endl;
	}

	Graph::DirectedWeightedGraph<double> graph(vertex_count);

	size_t count1 = 0;
	std::cout << "bus_to_edges list\n";
	for(const auto& [bus_number, e]: bus_to_edges){
		count1 += e.size();
		for(const auto& [from, to, distance]: e){
			std::cout << "from - " << from << ", to - " << to << "; distance - " << distance << std::endl;
			graph.AddEdge({from, to, distance});
		}
	}

	std::cout << "\nbuses.size() = " << buses.size() << "\n";
	std::cout << "stops.size() = " << stops.size() << "\n";
	std::cout << "vertex_count - " << vertex_count << "\n";
	std::cout << "bus_to_edges.size() = " << count1 << "\n";
	std::cout << "edges.size() = " << edges.size() << "\n";


	//return;
	std::cout << "edges list\n";
	for(const auto& [from, to, distance]: edges){
		std::cout << "from - " << from << ", to - " << to << "; distance - " << distance << std::endl;
		graph.AddEdge({from, to, distance});
	}
	//return;
	Graph::Router<double> router(graph);

	std::cout << "after router\n";
	//return;
	out << std::fixed << std::setprecision(6) << "[\n";

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
				out << "\t\t\"unique_stop_count\": " << it->second.GetUniqueSize() << ",\n";
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
			continue;
			if(rc.stop_from == rc.stop_to){
				std::cout << "\t\t\"total_time\": 0,\n\t\t\"items\": []\n";
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
	out << "\n]";*/
}

Route BusManager::BuildBestRoute(const RouteCommand& command,
			const Graph::DirectedWeightedGraph<double>& graph,
			Graph::Router<double>& router) const {

	throw std::invalid_argument("BusManager::BuildBestRoute not inpemented");
	/*std::vector<Graph::VertexId> vertex_from_list;
	if(auto it = stop_to_transfer.find(command.stop_from); it != stop_to_transfer.end()){
		std::copy(it->second.begin(), it->second.end(), std::back_inserter(vertex_from_list));
	} else {
		vertex_from_list = {stops.at(command.stop_from).id};
	}

	std::vector<Graph::VertexId> vertex_to_list;
	if(auto it = stop_to_transfer.find(command.stop_to); it != stop_to_transfer.end()){
		std::copy(it->second.begin(), it->second.end(), std::back_inserter(vertex_to_list));
	} else {
		vertex_to_list = {stops.at(command.stop_to).id};
	}

	Route route;
	route.total_time = -1.0;

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

				if(logging){
					std::cout << "Ищем маршрут " << command.stop_from << "(" << vertex_from << ")";
					std::cout << " - " << command.stop_to << "(" << vertex_to << ")\n";

					std::cout << "route_info->edge_count = " << route_info->edge_count << "\n";
					std::cout << "route_info->weight = " << route_info->weight << "\n";
					//return Route();
				}
			}
		}
	}

	if(route.total_time > 0){
		route.total_time += settings.bus_wait_time;

		RouteItemWait rw;
		rw.stop_name = command.stop_from;
		rw.bus_wait_time = settings.bus_wait_time;

		route.items.push_back(std::make_shared<RouteItemWait>(rw));
		size_t temp_span_count = 0;

		std::unordered_map<std::string, std::vector<std::string>> stops_to_unknow_buses;
		size_t last_edge_index = 0;

		for(const Graph::EdgeId edge_id: route_edges) {
			const Graph::Edge edge = graph.GetEdge(edge_id);

			std::string stop_name_from = stop_id_to_stops.at(edge.from);
			std::string stop_name_to = stop_id_to_stops.at(edge.to);

			size_t real_edge_from = edge.from;
			if(real_edge_from >= stops.size()){
				const Stop& stop_from = stops.at(stop_name_from);
				real_edge_from = stop_from.id;
			}

			size_t real_edge_to = edge.to;
			if(real_edge_to >= stops.size()){
				const Stop& stop_to = stops.at(stop_name_to);
				real_edge_to = stop_to.id;
			}

			if(real_edge_from == real_edge_to){
				RouteItemWait rw;
				rw.stop_name = stop_id_to_stops.at(edge.from);
				rw.bus_wait_time = settings.bus_wait_time;

				route.items.push_back(std::make_shared<RouteItemWait>(rw));
				temp_span_count = 0;
			} else {
				if(temp_span_count == 0){
					RouteItemBus rb;
					rb.bus_number = std::to_string(++last_edge_index);
					rb.span_count = ++temp_span_count;
					rb.bus_move_time = edge.weight;

					route.items.push_back(std::make_shared<RouteItemBus>(rb));
					stops_to_unknow_buses[std::to_string(last_edge_index)].push_back(stop_name_from);
				} else {
					RouteItemBus* back_bus = ((RouteItemBus*)route.items.back().get());
					back_bus->span_count = ++temp_span_count;
					back_bus->bus_move_time += edge.weight;
				}

				stops_to_unknow_buses[std::to_string(last_edge_index)].push_back(stop_name_to);
			}
		}

		std::unordered_map<std::string, std::string> stops_to_know_buses;
		for(const auto& [index, stops]: stops_to_unknow_buses){

			std::unordered_map<std::string, size_t> current_buses;
			for(const std::string stop_name: stops){
				const std::set<std::string>& buses_for_stop = stop_to_buses.at(stop_name);
				for(const std::string& bus_number: buses_for_stop) {
					current_buses[bus_number]++;
				}
			}

			bool line_route_type = false;
			size_t stops_count = stops.size();
			for(const auto& [bus_number, count]: current_buses){
				if(count < stops_count){
					continue;
				}

				const Bus& bus = buses.at(bus_number);
				if(bus.route_type == RouteType::Line){
					stops_to_know_buses[index] = bus_number;
					line_route_type = true;
					break;
				}
			}

			if(!line_route_type){
				for(const auto& [bus_number, count]: current_buses){
					if(count == stops_count){
						stops_to_know_buses[index] = bus_number;
						break;
					}
				}
			}
		}

		for(auto& item: route.items){
			if(item->GetType() == RouteItemType::Wait){
				continue;
			}

			RouteItemBus* item_bus = (RouteItemBus*)item.get();
			item_bus->bus_number = stops_to_know_buses.at(item_bus->bus_number);
		}
	}

	return route;*/
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

		std::string stop_name_1 = bus.stops[first_stop];
		std::string stop_name_2 = bus.stops[second_stop];

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

	auto it_end = stops.end();
	size_t i = 0;
	do{
		std::string stop_name_1 = bus.stops[i];
		std::string stop_name_2 = bus.stops[i+1];

		auto it1 = stops.find(stop_name_1);
		if(it1 == it_end){
			throw std::invalid_argument(stop_name_1 + " not found");
		}

		auto it2 = stops.find(stop_name_2);
		if(it2 == it_end){
			throw std::invalid_argument(stop_name_2 + " not found");
		}

		result += Distance(it1->second.point, it2->second.point).Length();
		i++;
	} while(i < stop_count - 1);

	if(bus.route_type == RouteType::Line){
		result *= 2;
	}

	return result;
}

void BusManager::FillEdges(const Bus& bus){
	if(bus.route_type == RouteType::Line){
		FillEdgesLine(bus);
		//FillEdgesLinePrepare(bus, false);
	} else {
		FillEdgesRound(bus);
	}
}

void BusManager::FillEdgesLine(const Bus& bus){
	const std::vector<std::string>& stops_for_bus = bus.stops;
	size_t stop_count =	stops_for_bus.size();

	auto it_end = stop_distances.end();
	for(size_t i = 0; i < stop_count-1; ++i ){
		std::string stop_name_1 = stops_for_bus[i];
		std::string stop_name_2 = stops_for_bus[i+1];

		auto it_stop_1_2 = stop_distances.find({stop_name_1, stop_name_2});
		auto it_stop_2_1 = stop_distances.find({stop_name_2, stop_name_1});

		if(it_stop_1_2 == it_end && it_stop_2_1 == it_end){
			throw std::invalid_argument("distance [" + stop_name_1 + ", " + stop_name_2 + "] not found");
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

		size_t vertex_1 = last_init_id++;
		size_t vertex_2 = last_init_id++;

		vertex_to_bus_stop.insert({vertex_1, {bus.name, stop_name_1}});
		vertex_to_bus_stop.insert({vertex_2, {bus.name, stop_name_2}});

		edges.push_back({vertex_1, vertex_2, distance_1_2});
		edges.push_back({vertex_2, vertex_1, distance_2_1});

		if(auto it = stop_to_vertex.find(stop_name_1); it != stop_to_vertex.end()){
			for(const size_t other_vertex_id: it->second){
				edges.push_back({vertex_1, other_vertex_id, settings.bus_wait_time});
				edges.push_back({other_vertex_id, vertex_1, settings.bus_wait_time});
			}
		}

		if(auto it = stop_to_vertex.find(stop_name_2); it != stop_to_vertex.end()){
			for(const size_t other_vertex_id: it->second){
				edges.push_back({vertex_2, other_vertex_id, settings.bus_wait_time});
				edges.push_back({other_vertex_id, vertex_2, settings.bus_wait_time});
			}
		}

		stop_to_vertex[stop_name_1].insert(vertex_1);
		stop_to_vertex[stop_name_2].insert(vertex_2);
	}
}

void BusManager::FillEdgesRound(const Bus& bus){
	//const std::vector<std::string>& stops_for_bus = bus.stops;
	//size_t stop_count =	stops_for_bus.size();

	throw std::invalid_argument("BusManager::FillEdgesLinePrepare not inpemented");

	/*std::unordered_map<std::string, int> stop_to_count;
	for(const std::string stop_name: stops_for_bus){
		stop_to_count[stop_name]++;
	}

	std::vector<Graph::Edge<double>>& e = bus_to_edges[bus.name];
	auto it_end = stop_distances.end();

	for(size_t i = 0; i < stop_count; i++){
		std::string stop_name_cur = stops_for_bus[i];

		if(i == stop_count - 1){
			std::string stop_name_next = stops_for_bus[0];
			double distance = settings.bus_wait_time;
			e.push_back({i, 0, distance});
		} else {
			std::string stop_name_next = stops_for_bus[i + 1];

			auto it = stop_distances.find({stop_name_cur, stop_name_next});
			if(it == it_end){
				it = stop_distances.find({stop_name_next, stop_name_cur});
				if(it == it_end){
					throw std::invalid_argument("distance [" + stop_name_cur + ", " + stop_name_next + "] not found");
				}
			}

			double distance = it->second / settings.bus_velocity;
			e.push_back({i, i + 1, distance});

			if(i > 0 && stop_to_count.at(stop_name_cur) == 2){
				e.push_back({i, i, double(settings.bus_wait_time)});
			}
		}
	}*/
}

