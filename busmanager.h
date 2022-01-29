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
	size_t last_init_id;

	//Название остановки с пересадкой -> несколько фиктивных остановок.
	//Для эмуляции переходов между разными маршрутами
	std::unordered_map<std::string, std::vector<size_t>> stop_to_transfer;

	std::unordered_map<std::string, std::vector<size_t>> stop_to_transfer_for_round;

	bool logging = false;

	std::vector<Graph::Edge<double>> edges;
	std::unordered_map<std::string, std::vector<Graph::Edge<double>>> bus_to_edges;

	RoutingSettings settings;
	std::unordered_map<std::string, Bus> buses;
	std::unordered_map<std::string, Stop> stops;
	std::unordered_map<std::string, std::set<std::string>> stop_to_buses;
	std::unordered_map<StopPair, size_t, StopPairHasher> stop_distances;

	std::vector<std::unique_ptr<Command>> commands;

	std::unordered_map<Graph::VertexId, std::set<std::string>> stop_id_to_buses;
	std::unordered_map<Graph::VertexId, std::string> stop_id_to_stops;

public:
	BusManager& Read(std::istream& in = std::cin);
	void WriteResponse(std::ostream& out = std::cout) const;

private:
	double GetDistanceByGeo(const Bus& bus) const;
	size_t GetDistanceByStops(const Bus& bus) const;
	size_t GetDistanceByStops(const Bus& bus, bool forward) const;

	BusManager& ReadData(const std::vector<Json::Node>& node);
	BusManager& ReadRequest(const std::vector<Json::Node>& node);
	BusManager& ReadSettings(const std::map<std::string, Json::Node>& node);

	Route BuildBestRoute(const RouteCommand& command,
		const Graph::DirectedWeightedGraph<double>& graph,
		Graph::Router<double>& router) const;

	void FillEdgesPrepare(const Bus& bus);
	void FillEdgesRoundPrepare(const Bus& bus);
	void FillEdgesLinePrepare(const Bus& bus, bool forward);

	double GetDistance(std::vector<std::string>::const_iterator it) const;

};
