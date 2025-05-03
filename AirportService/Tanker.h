#ifndef TANKER_H
#define TANKER_H
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

#include "../Airport/Airport.h"

class Airport;
using namespace std;

class Tanker {
    void refill()
    {
        isRefilling = true;
        std::this_thread::sleep_for(chrono::seconds(3));
        fuelLevel = fuelCapacity;
        isRefilling = false;
    }
public:
    int fuelCapacity = 150;
    int fuelLevel = 150;
    bool isRefilling = false;

    void run(Airport* airport, int tankTimeInMilliseconds);
};



#endif //TANKER_H
