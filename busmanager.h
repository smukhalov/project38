#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <set>
#include <unordered_set>
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
	bool logging = true;

	struct BusVertex{
		std::string bus_name;
		size_t vertex_id;

		bool operator == (const BusVertex& other) const {
			return bus_name == other.bus_name && vertex_id == other.vertex_id;
		}
	};

	struct BusVertexHasher {
		size_t operator() (const BusVertex& bv) const {
			size_t x = 2'946'901;
			return shash(bv.bus_name) * x + size_t_hash(bv.vertex_id);
		}

		std::hash<std::string> shash;
		std::hash<size_t> size_t_hash;
	};

	struct BusStop{
		std::string bus_name;
		size_t stop_id;
		std::string stop_name;

		bool operator == (const BusStop& other) const {
			return bus_name == other.bus_name && stop_id == other.stop_id;
		}
	};

	struct BusStopHasher {
		size_t operator() (const BusStop& bs) const {
			size_t x = 2'946'901;
			return shash(bs.bus_name) * x + size_t_hash(bs.stop_id);
		}

		std::hash<std::string> shash;
		std::hash<size_t> size_t_hash;
	};

	std::unordered_map<size_t, std::unordered_set<BusStop, BusStopHasher>> vertex_to_bus_stop;
	std::unordered_map<BusStop, size_t, BusStopHasher> bus_stop_to_vertex;

	std::unordered_map<std::string, std::unordered_set<BusVertex, BusVertexHasher>> stop_to_bus_vertex;

	struct Edge {
	    size_t from;
	    size_t to;
	    double distance;
	    RouteItemType route_item_type;

	    bool operator == (const Edge& other) const {
			return from == other.from && to == other.to;
		}
	  };

	struct EdgeHasher {
		size_t operator() (const Edge& e) const {
			size_t x = 2'946'901;
			return shash(e.from) * x + shash(e.to);
		}

		std::hash<std::size_t> shash;
	};

	std::unordered_set<Edge, EdgeHasher> edges;

	RoutingSettings settings;
	std::unordered_map<std::string, Bus> buses;
	std::unordered_map<std::string, Stop> stops;
	std::unordered_map<std::string, std::set<std::string>> stop_to_buses;
	std::unordered_map<StopPair, size_t, StopPairHasher> stop_distances;

	std::vector<std::unique_ptr<Command>> commands;

	double GetDistanceByGeo(const Bus& bus) const;
	size_t GetDistanceByStops(const Bus& bus) const;
	size_t GetDistanceByStops(const Bus& bus, bool forward) const;

	BusManager& ReadData(const std::vector<Json::Node>& node);
	BusManager& ReadRequest(const std::vector<Json::Node>& node);
	BusManager& ReadSettings(const std::map<std::string, Json::Node>& node);

	Route BuildBestRoute(const RouteCommand& command,
		const Graph::DirectedWeightedGraph<double>& graph,
		Graph::Router<double>& router) const;

	void FillEdgesLine(const Bus& bus);
	void FillEdgesRound(const Bus& bus);

	double GetDistance(std::vector<std::string>::const_iterator it) const;
	void AddEdge(const Edge& edge);

};
