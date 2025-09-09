#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "Person.h"
#include "Ride.h"
#include <vector>
#include <memory>
#include <iostream>

class Dispatcher {
    std::vector<User> users;
    std::vector<Driver> drivers;
    int rideCounter = 1;
public:
    void addUser(const User &u) { users.push_back(u); }
    void addDriver(const Driver &d) { drivers.push_back(d); }

    std::unique_ptr<Ride> bookRide(int userId, Coord s, Coord e, FareStrategy* fs) {
        User* u = nullptr;
        for(auto &usr: users) if(usr.getId()==userId) u = &usr;

        Driver* d = nullptr;
        for(auto &drv: drivers) if(drv.isAvailable()) { d=&drv; break; }

        if(!u || !d) {
            std::cout << "No match found\n";
            return nullptr;
        }

        d->setAvailable(false);
        return std::make_unique<Ride>(rideCounter++, u, d, s, e, fs);
    }
};

#endif
