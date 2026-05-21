#pragma once
#include "GraphCore.h"
#include <string>
#include <vector>

enum TransportType { BUS, TRAM };

class Passenger {
public:
    int destinationStop;

    Passenger(int dest) : destinationStop(dest) {}
};

class Route {
public:
    string number;
    TransportType type;
    vector<int> stopIndices;
    vector<double> travelTimes;

    Route(const string& num, TransportType t) : number(num), type(t) {}

    void addStop(int stopIndex, double travelTimeFromPrevious = 0) {
        stopIndices.push_back(stopIndex);
        travelTimes.push_back(travelTimeFromPrevious);
    }

    bool servesStop(int stopIndex) const {
        for (int s : stopIndices) {
            if (s == stopIndex) return true;
        }
        return false;
    }

    bool isBus() const { return type == BUS; }
    bool isTram() const { return type == TRAM; }
};

class Vehicle {
public:
    string id;
    Route* route;
    int currentStopIndex;
    int currentStopVertex;
    double minutesToNextStop;
    int passengers;
    int capacity;
    vector<Passenger*> passengersInside;
    int completedCycles;

    Vehicle(const string& _id, int _capacity, Route* _route)
        : id(_id), capacity(_capacity), route(_route),
        currentStopIndex(0),
        currentStopVertex(0),
        minutesToNextStop(0),
        passengers(0),
        completedCycles(0)
    {
        if (route && !route->stopIndices.empty()) {
            currentStopVertex = route->stopIndices[0];
        }
    }

    void moveToNextStop() {
        if (!route || route->stopIndices.empty()) return;

        
        int fromIndex = currentStopIndex;

        
        currentStopIndex = (currentStopIndex + 1) % route->stopIndices.size();
        currentStopVertex = route->stopIndices[currentStopIndex];

        if (currentStopIndex == 0 && fromIndex == route->stopIndices.size() - 1) {
            completedCycles++; 
        }
        if (fromIndex < route->travelTimes.size()) {
            minutesToNextStop = route->travelTimes[fromIndex];
        }
        else {
            minutesToNextStop = 3;
        }
    }
};

