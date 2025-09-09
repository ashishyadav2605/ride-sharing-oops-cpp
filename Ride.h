#ifndef RIDE_H
#define RIDE_H

#include "Person.h"
#include "Utils.h"
#include <iostream>

class FareStrategy {
public:
    virtual double calculateFare(double dist) = 0;
    virtual ~FareStrategy() = default;
};

class StandardFare : public FareStrategy {
public:
    double calculateFare(double dist) override {
        return 10 + 8 * dist;
    }
};

class PremiumFare : public FareStrategy {
public:
    double calculateFare(double dist) override {
        return 20 + 15 * dist;
    }
};

class Ride {
    int rideId;
    const User* user;
    Driver* driver;
    Coord start, end;
    double fare;
public:
    Ride(int id, const User* u, Driver* d, Coord s, Coord e, FareStrategy* fs)
        : rideId(id), user(u), driver(d), start(s), end(e) {
        double dist = distance(s,e);
        fare = fs->calculateFare(dist);
    }
    void complete() {
        driver->setAvailable(true);
    }
    void showDetails() const {
        std::cout << "Ride " << rideId << " User: " << user->getName()
                  << " Driver: " << driver->getName()
                  << " Fare: " << fare << "\n";
    }
};

#endif
