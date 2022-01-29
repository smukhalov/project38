#pragma once
#include <string>

enum class CommandType {
	None,
	Bus,
	Stop,
	Route
};

struct Command {
	size_t id;
	virtual CommandType GetType() const = 0;
	virtual ~Command() = default;
};

struct BusCommand: Command {
	std::string name;
	CommandType GetType() const override {
		return CommandType::Bus;
	}
};

struct StopCommand: Command {
	std::string name;
	CommandType GetType() const override {
			return CommandType::Stop;
		}
};

struct RouteCommand: Command {
	std::string stop_from;
	std::string stop_to;

	CommandType GetType() const override {
		return CommandType::Route;
	}
};
