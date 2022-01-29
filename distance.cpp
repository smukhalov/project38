#include <cmath>
#include "distance.h"

Distance::Distance(const Point& lhs_, const Point& rhs_): lhs(lhs_), rhs(rhs_), length(Calculate()) {
	}

const double Distance::Length() const {
	return length;
}

const double Distance::Calculate() const {
	//в радианах
	double lhs_latitude = lhs.latitude * RAD_IN_GRAD;
	double lhs_longitude = lhs.longitude * RAD_IN_GRAD;

	double rhs_latitude = rhs.latitude * RAD_IN_GRAD;
	double rhs_longitude = rhs.longitude * RAD_IN_GRAD;

	//косинусы и синусы широт и разницы долгот
	double cl1 = cos(lhs_latitude);
	double cl2 = cos(rhs_latitude);
	double sl1 = sin(lhs_latitude);
	double sl2 = sin(rhs_latitude);
	double delta = rhs_longitude - lhs_longitude;
	double cdelta = cos(delta);
	double sdelta = sin(delta);

	//вычисления длины большого круга
	double y = sqrt(pow(cl2 * sdelta, 2) + pow(cl1 * sl2 - sl1 * cl2 * cdelta, 2));
	double x = sl1 * sl2 + cl1 * cl2 * cdelta;
	double ad = atan2(y, x);

	return ad * RADIUS;

	//return acos(sin(lhs_latitude) * sin(rhs_latitude) + cos(lhs_latitude) * cos(rhs_latitude) * cos(abs(lhs_longitude - rhs_longitude))) * RADIUS;
}


