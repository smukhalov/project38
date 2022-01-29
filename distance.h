#pragma once
#include <iostream>

const size_t RADIUS = 6371000;
const double PI = 3.1415926535;
const double RAD_IN_GRAD = PI / double(180.0);

enum class RouteType{
	Round,
	Line
};

struct Point {
	double latitude;
	double longitude;
};

class Distance {
	Point lhs;
	Point rhs;
	double length;
public:
	Distance(const Point& lhs_, const Point& rhs_);
	const double Length() const;

private:
	const double Calculate() const;
};

