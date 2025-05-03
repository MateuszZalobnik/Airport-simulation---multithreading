#ifndef RUNWAYMANAGER_H
#define RUNWAYMANAGER_H
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <mutex>
#include <vector>
#include "../Airplane/Airplane.h"
#include <iostream>
#include <thread>
#include <random>
#include <windows.h>
#include <iomanip>
#include <algorithm>

using namespace std;

struct Runway
{
    int runwayPermission = 0;
    bool isLanding = false;
    bool isTakingOff = false;
    Airplane* airplane = nullptr;
};

struct Gate
{
    mutex mtx;
    bool isAvailable = true;
    Airplane* airplane = nullptr;
};
class Tanker;

class Airport
{
private:
    int tankTimeInMilliseconds = 500;
    int flightNumber = 0;
public:
    vector<Airplane*> airplanesInFlight;
    vector<Airplane*> airplanesInService;
    vector<Airplane*> airplanesWaiting;
    vector<Runway*> runways;
    vector<unique_ptr<Gate>> gates;
    vector<Tanker*> tankers;

    mutex queueMutex;

    vector<thread> threads;

    Airport(int numRunways, int numGates, int numTankers);
private:
    void airplaneGenerator();
    void runRunway(Runway* runway);
    bool isGateAvailable();
    void manageLanding(Runway* runway);
    void manageTakingOff(Runway* runway);
    void runTanker(Tanker* tanker);
    void displayLoop();
    void displayState();
    void displayRunways();
    void displayGates();
    void displayTankers();
    void StatusChecker();
};


#endif //RUNWAYMANAGER_H
