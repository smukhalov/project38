#pragma once
#include <string>
#include <vector>
#include <memory>

enum class RouteItemType {
	Wait,
	Bus
};

struct RouteItem {
	virtual RouteItemType GetType() const = 0;
	virtual void Print(std::ostream& out) const = 0;
	virtual ~RouteItem() = default;
};

struct Route {
	double total_time;
	std::vector<std::shared_ptr<RouteItem>> items;
};

struct RouteItemWait: RouteItem {
	RouteItemType GetType() const override {
		return RouteItemType::Wait;
	}
	std::string stop_name;
	uint32_t bus_wait_time;

	void Print(std::ostream& out) const override {
		out << "\t\t\t{\n";
		out << "\t\t\t\t\"type\": \"Wait\",\n";
		out << "\t\t\t\t\"stop_name\": \"" << stop_name << "\",\n";
		out << "\t\t\t\t\"time\": " << bus_wait_time << "\n";
		out << "\t\t\t}";
	}
};

struct RouteItemBus: RouteItem {
	RouteItemType GetType() const override {
		return RouteItemType::Bus;
	}
	std::string bus_number;
	uint32_t span_count; //Число остановок
	double bus_move_time; //В минутах

	void Print(std::ostream& out) const override {
		out << "\t\t\t{\n";
		out << "\t\t\t\t\"type\": \"Bus\",\n";
		out << "\t\t\t\t\"bus\": \"" << bus_number << "\",\n";
		out << "\t\t\t\t\"span_count\": " << span_count << ",\n";
		out << "\t\t\t\t\"time\": " << bus_move_time << "\n";
		out << "\t\t\t}";
	}
};

struct StopPair {
	std::string stop_from;
	std::string stop_to;

	bool operator == (const StopPair& other) const {
		return stop_from == other.stop_from && stop_to == other.stop_to;
	}
};

struct StopPairHasher {
	size_t operator() (const StopPair& sp) const {
		size_t x = 2'946'901;
		return shash(sp.stop_from) * x + shash(sp.stop_to);
	}

	std::hash<std::string> shash;
};

struct StopIdPair {
	size_t stop_from;
	size_t stop_to;

	bool operator == (const StopIdPair& other) const {
		return stop_from == other.stop_from && stop_to == other.stop_to;
	}
};

struct StopIdPairHasher {
	size_t operator() (const StopIdPair& sp) const {
		size_t x = 2'946'901;
		return shash(sp.stop_from) * x + shash(sp.stop_to);
	}

	std::hash<size_t> shash;
};


