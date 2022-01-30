#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include "bus.h"
#include "command.h"
#include "route.h"
#include "json.h"
#include "routing_settings.h"
#include "graph.h"
#include "router.h"

class BusManager {
public:
	BusManager();
	BusManager& Read(std::istream& in = std::cin);
	void WriteResponse(std::ostream& out = std::cout) const;

private:
	size_t last_init_id;

	struct BusStop{
		std::string bus_number;
		std::string stop_name;

		/*bool operator == (const BusStop& other) const {
			return stop_name == other.stop_name && bus_number == other.bus_number;
		}*/
	};

	std::unordered_map<size_t, BusStop> vertex_to_bus_stop;
	std::unordered_map<std::string, std::set<size_t>> stop_to_vertex;

	/*struct BusStopHasher {
	  	size_t operator() (const BusStop& bs) const {
	  		size_t x = 2'946'901;
	  		return shash(bs.stop_name) * x + shash(bs.bus_number);
	  	}

	  	std::hash<std::string> shash;
	  };*/


	//std::unordered_map<BusStop, int, BusStopHasher> bus_stop_to_vertex;

	//Название остановки с пересадкой -> несколько фиктивных остановок.
	//Для эмуляции переходов между разными маршрутами
	//std::unordered_map<std::string, std::vector<size_t>> stop_to_transfer;

	//std::unordered_map<std::string, std::vector<size_t>> stop_to_transfer_for_round;

	//ool logging = false;

	std::vector<Graph::Edge<double>> edges;
	//std::unordered_map<std::string, std::vector<Graph::Edge<double>>> bus_to_edges;

	RoutingSettings settings;
	std::unordered_map<std::string, Bus> buses;
	std::unordered_map<std::string, Stop> stops;
	std::unordered_map<std::string, std::set<std::string>> stop_to_buses;
	std::unordered_map<StopPair, size_t, StopPairHasher> stop_distances;

	std::vector<std::unique_ptr<Command>> commands;

	//std::unordered_map<Graph::VertexId, std::set<std::string>> stop_id_to_buses;
	//std::unordered_map<Graph::VertexId, std::string> stop_id_to_stops;

	double GetDistanceByGeo(const Bus& bus) const;
	size_t GetDistanceByStops(const Bus& bus) const;
	size_t GetDistanceByStops(const Bus& bus, bool forward) const;

	BusManager& ReadData(const std::vector<Json::Node>& node);
	BusManager& ReadRequest(const std::vector<Json::Node>& node);
	BusManager& ReadSettings(const std::map<std::string, Json::Node>& node);

	Route BuildBestRoute(const RouteCommand& command,
		const Graph::DirectedWeightedGraph<double>& graph,
		Graph::Router<double>& router) const;

	void FillEdges(const Bus& bus);
	void FillEdgesRound(const Bus& bus);
	void FillEdgesLine(const Bus& bus);

	double GetDistance(std::vector<std::string>::const_iterator it) const;

	//void FillGrapth(size_t vertex_id, std::string stop_name, double distance);
};
