#include "Airport.h"
#include "../AirportService/Tanker.h"

Airport::Airport(int numRunways, int numGates, int numTankers)
{
    for (int i = 0; i < numRunways; ++i)
        runways.push_back(new Runway());
    for (int i = 0; i < numGates; ++i)
        gates.push_back(make_unique<Gate>());

    for (int i = 0; i < numTankers; ++i)
    {
        tankers.push_back(new Tanker());
    }

    thread airplaneThread(&Airport::airplaneGenerator, this);
    thread displayThread(&Airport::displayLoop, this);
    thread statusCheckerThread(&Airport::StatusChecker, this);
    thread generatePassengerGroupsThread(&Airport::GeneratePassengerGroups, this);
    thread seatPassengersThread(&Airport::SeatPassengersThread, this);

    for (Tanker* t : tankers) {
        threads.emplace_back(&Airport::runTanker, this, t);
    }
    // jeden wątek dla każdego pasa
    for (Runway* r : runways) {
        threads.emplace_back(&Airport::runRunway, this, r);
    }


    airplaneThread.join();
    displayThread.join();
    statusCheckerThread.join();
    generatePassengerGroupsThread.join();
    seatPassengersThread.join();
    for (auto& t : threads) {
        t.join();
    }
}

void Airport::airplaneGenerator()
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 4);
    uniform_int_distribution<> distType(0, 3);

    while (true)
    {
        this_thread::sleep_for(chrono::seconds(dist(gen)));
        auto airplaneType = (AirplaneType)distType(gen);
        auto* newPlane = new Airplane(flightNumber, airplaneType);
        {
            lock_guard<mutex> lock(this->queueMutex);
            this->airplanesInFlight.push_back(newPlane);
            flightNumber++;
        }
    }
}

void Airport::runRunway(Runway* runway)
{
    while (true)
    {
        Airplane* airplane = nullptr;

        {
            if (!isGateAvailable())
            {
                manageTakingOff(runway);
            }
            else if (runway->runwayPermission % 2 == 0)
            {
                manageLanding(runway);
            }
            else
            {
                manageTakingOff(runway);
            }
            runway->runwayPermission++;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

bool Airport::isGateAvailable()
{
    for (auto& gate : this->gates)
    {
        if (gate->isAvailable)
        {
            return true;
        }
    }
    return false;
}

void Airport::manageLanding(Runway* runway)
{
    Airplane* airplane = nullptr;

    {
        lock_guard<mutex> lock(this->queueMutex);
        if (!this->airplanesInFlight.empty())
        {
            airplane = this->airplanesInFlight.front();
            this->airplanesInFlight.erase(this->airplanesInFlight.begin());
        }
    }

    if (!airplane)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        return;;
    }

    Gate* assignedGate = nullptr;

    runway->isLanding = true;
    runway->airplane = airplane;

    while (!assignedGate)
    {
        for (auto& gate : this->gates)
        {
            if (gate->mtx.try_lock())
            {
                assignedGate = gate.get();
                break;
            }
        }
        if (!assignedGate)
            this_thread::sleep_for(chrono::milliseconds(100));
    }

    playSoundFile("D:\\school\\SO2_Lot\\landingv2.mp3");
    this_thread::sleep_for(chrono::seconds(runway->airplane->TimeToTakeOffAndLandingInSeconds));

    {
        lock_guard<mutex> lock(this->queueMutex);
        this->airplanesInService.push_back(airplane);
    }

    assignedGate->isAvailable = false;
    assignedGate->airplane = airplane;
    runway->isLanding = false;
    runway->airplane = nullptr;
    airplane->landingTime = chrono::steady_clock::now();
}

void Airport::manageTakingOff(Runway* runway)
{
    Airplane* airplane = nullptr;

    {
        lock_guard<mutex> lock(queueMutex);
        if (!airplanesWaiting.empty())
        {
            airplane = airplanesWaiting.front();
            airplanesWaiting.erase(airplanesWaiting.begin());
        }
    }

    if (!airplane)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        return;;
    }

    runway->isTakingOff = true;
    runway->airplane = airplane;
    airplane->actualTakeOffTime = chrono::steady_clock::now();
    auto serviceDuration = chrono::duration_cast<chrono::seconds>(airplane->readyToTakeOffTime - airplane->landingTime).count();
    auto waitTime = chrono::duration_cast<chrono::seconds>(airplane->actualTakeOffTime - airplane->readyToTakeOffTime).count();
    totalPlaneServiceTime += serviceDuration;
    totalPlaneWaitTime += waitTime;
    totalPlaneCount++;

    this_thread::sleep_for(chrono::seconds(runway->airplane->TimeToTakeOffAndLandingInSeconds));
    playSoundFile("D:\\school\\SO2_Lot\\landingv2.mp3");

    Gate* airplaneGate;
    for (auto& gate : this->gates)
    {
        if (gate->airplane && gate->airplane->flightNumber == airplane->flightNumber)
        {
            airplaneGate = gate.get();
            break;
        }
    }
    airplaneGate->isAvailable = true;
    airplaneGate->airplane = nullptr;
    airplaneGate->mtx.unlock();
    runway->isTakingOff = false;
    runway->airplane = nullptr;
}

void Airport::runTanker(Tanker* tanker)
{
    tanker->run(this, tankTimeInMilliseconds);
}

void Airport::displayLoop()
{
    while (true)
    {
        displayState();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void Airport::playSoundFile(const std::string& filename) {
    sf::SoundBuffer* buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile(filename)) {
        delete buffer;
        return;
    }

    sf::Sound* sound = new sf::Sound();
    sound->setBuffer(*buffer);
    sound->play();

    // Czekamy, aż dźwięk się skończy, potem czyścimy
    std::thread([sound, buffer]() {
        while (sound->getStatus() == sf::Sound::Playing)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        delete sound;
        delete buffer;
    }).detach();
}

void Airport::displayState()
{
    {
        lock_guard<mutex> lock(queueMutex);
        cout << endl;
        cout << endl;
        cout << "========================================================================================";
        cout << endl;
        cout << "in flight: " << airplanesInFlight.size() << endl;
        cout << "in service: " << airplanesInService.size() << endl;
        cout << "waiting: " << airplanesWaiting.size() << endl;
        cout << endl;

        displayStatistics();
        displayRunways();
        displayTankers();
        displayGates();
        displaySeats();


        cout << endl;
        cout << "========================================================================================";
        cout << endl;
        cout << endl;
    }
}

void Airport::displayStatistics()
{
    if (totalPlaneCount > 0)
    {
        cout << endl;
        cout << "Average service time for planes: "
             << (totalPlaneServiceTime / totalPlaneCount) << "s" << endl;

        cout << "Average wait time for planes: "
                << (totalPlaneWaitTime / totalPlaneCount) << "s" << endl;

    }
}

void Airport::displayRunways()
{
    cout << "Runways: ";
    cout << endl;
    cout << "+";
    for (int i = 0; i < runways.size(); ++i)
    {
        cout << "----+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < runways.size(); ++i)
    {
        cout << " " << setw(2) << i << " |";
    }
    cout << "  <- Runway ID" << endl;

    cout << "+";
    for (int i = 0; i < runways.size(); ++i)
    {
        cout << "----+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < runways.size(); ++i)
    {
        char status = runways[i]->isLanding ? 'L' : (runways[i]->isTakingOff ? 'T' : 'F');
        cout << " " << setw(2) << status << " |";
    }
    cout << "  <- F: free, T: taking off, L: landing" << endl;

    cout << "+";
    for (int i = 0; i < runways.size(); ++i)
    {
        cout << "----+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < runways.size(); ++i)
    {
        auto flightNumber = runways[i]->airplane ? to_string(runways[i]->airplane->flightNumber) : "-";
        cout << " " << setw(2) << flightNumber << " |";
    }
    cout << "  <- Flight Number" << endl;

    cout << "+";
    for (int i = 0; i < runways.size(); ++i)
    {
        cout << "----+";
    }
    cout << endl;
}

void Airport::displayGates()
{
    cout << "Gates: ";
    cout << endl;
    cout << "+";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << " " << setw(10) << i << " |";
    }
    cout << "  <- Gate ID" << endl;

    cout << "+";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < gates.size(); ++i)
    {
        char status = gates[i]->isAvailable ? 'F' : 'B';
        cout << " " << setw(10) << status << " |";
    }
    cout << "  <- F: free, B: busy" << endl;

    cout << "+";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < gates.size(); ++i)
    {
        auto flightNumber = gates[i]->airplane ? to_string(gates[i]->airplane->flightNumber) : "-";
        cout << " " << setw(10) << flightNumber << " |";
    }
    cout << "  <- Flight number" << endl;

    cout << "+";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < gates.size(); ++i)
    {
        auto airplane = gates[i]->airplane;
        auto fuelStatus = airplane ? to_string(airplane->currentFuel) + "/" + to_string(airplane->tankCapacity) : "-";
        cout << " " << setw(10) << fuelStatus << " |";
    }
    cout << "  <- fuel status" << endl;

    cout << "+";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < gates.size(); ++i)
    {
        int allPassenegers = 0;
        auto passengerGroup = gates[i]->passengerGroups;
        for (int j = 0; j < passengerGroup.size(); ++j)
        {
            allPassenegers += passengerGroup[j];
        }

        cout << " " << setw(10) << allPassenegers << " |";
    }
    cout << "  <- all passenegers" << endl;

    cout << "+";
    for (int i = 0; i < gates.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;
}

void Airport::displayTankers()
{
    cout << "Tankers: ";
    cout << endl;
    cout << "+";
    for (int i = 0; i < tankers.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < tankers.size(); ++i)
    {
        cout << " " << setw(10) << i << " |";
    }
    cout << "  <- Tanker ID" << endl;

    cout << "+";
    for (int i = 0; i < tankers.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;

    cout << "|";
    for (int i = 0; i < tankers.size(); ++i)
    {
        auto fuelStatus = to_string(tankers[i]->fuelLevel) + "/" + to_string(tankers[i]->fuelCapacity);
        cout << " " << setw(10) << fuelStatus << " |";
    }
    cout << "  <- fuel status" << endl;

    cout << "+";
    for (int i = 0; i < tankers.size(); ++i)
    {
        cout << "------------+";
    }
    cout << endl;
}

void Airport::displaySeats()
{
    cout << endl;
    vector<Airplane*> planesToDisplay;

    // Zbieramy samoloty z bramek
    for (const auto& gate : gates) {
        if (gate->airplane != nullptr) {
            planesToDisplay.push_back(gate->airplane);
        }
    }

    if (planesToDisplay.empty()) {
        cout << "[INFO] Brak samolotów przypisanych do bramek.\n";
        return;
    }

    int maxRows = 0;
    for (auto* plane : planesToDisplay) {
        maxRows = max(maxRows, plane->rows);
    }
    for (int i = 0; i < gates.size(); ++i)
    {
        string gateId = "Gate ID: " + to_string(i);
        cout << left << setw (30) << gateId;
    }
    cout << "\n";

    for (int row = 0; row < maxRows; ++row) {
        for (auto& gate : gates) {
            if (!gate->airplane) {
                cout << left << setw(30) << " ";
                continue;
            }

            Airplane* plane = gate->airplane;
            lock_guard lock(plane->seatMutex);

            if (row < plane->rows) {
                string rowToDisplay = "| ";
                for (int seat = 0; seat < plane->seatsPerRow; ++seat) {
                    char symbol = plane->seats[row][seat] ? 'x' : '-';
                    rowToDisplay += symbol;
                    if (seat < plane->seatsPerRow - 1) {
                        rowToDisplay += " ";
                    }
                }
                rowToDisplay += " |";
                cout << left << setw(30) << rowToDisplay;
            } else {
                cout << left << setw(30) << " ";
            }
        }
        cout << "\n";
    }
}

void Airport::StatusChecker()
{
    while (true)
    {
        vector<Airplane*> toMove;

        {
            lock_guard<mutex> lock(queueMutex);

            for (Airplane* airplane : airplanesInService)
            {
                auto allSeatsTaken = true;
                auto seats = airplane->getSeats();
                for (int i = 0; i < airplane->rows; ++i)
                {
                    for (int j = 0; j < airplane->seatsPerRow; ++j)
                    {
                        if (!seats[i][j])
                        {
                            allSeatsTaken = false;
                            break;
                        }
                    }
                }

                if (airplane->currentFuel == airplane->tankCapacity && !airplane->isRefueling && allSeatsTaken)
                {
                    toMove.push_back(airplane);
                }
            }

            for (Airplane* airplane : toMove)
            {
                airplanesInService.erase(remove(airplanesInService.begin(), airplanesInService.end(), airplane),
                                         airplanesInService.end());
                airplanesWaiting.push_back(airplane);
                airplane->readyToTakeOffTime = chrono::steady_clock::now();
            }
        }

        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

void Airport::GeneratePassengerGroups()
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> groupSizeDist(1, 5);
    uniform_int_distribution<> groupCountDist(1, 4);

    while (true)
    {
        this_thread::sleep_for(chrono::milliseconds(3000));
        for (auto& gate : gates)
        {
            if (!gate) continue;

            lock_guard lock(gate->passengerMtx);

            int numGroups = groupCountDist(gen);

            for (int i = 0; i < numGroups; ++i)
            {
                int groupSize = groupSizeDist(gen);
                gate->passengerGroups.push_back(groupSize);
            }
        }
    }
}

void Airport::SeatPassengersThread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // opóźnienie między próbami

        for (auto& gate : gates) {
            if (!gate) continue;

            std::lock_guard<std::mutex> gateLock(gate->passengerMtx);

            Airplane* airplane = gate->airplane;
            if (!airplane) continue;
            if (gate->passengerGroups.empty()) continue;

            // przetwarzamy grupy po kolei
            std::vector<int> remainingGroups;

            for (int groupSize : gate->passengerGroups) {
                bool seated = airplane->tryTakeSeat(groupSize);
                if (!seated) {
                    remainingGroups.push_back(groupSize);
                } else {
                }
            }

            // Podmieniamy kolejkę pasażerów na pozostałe grupy
            gate->passengerGroups = std::move(remainingGroups);
        }
    }
}

