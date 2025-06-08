#ifndef AIRPLANE_H
#define AIRPLANE_H
#include <thread>

using namespace std;

enum AirplaneType
{
    SMALL = 0,
    MEDIUM = 1,
    LARGE = 2,
    MAX = 3,
};

class Airplane {
public:
    int seatsPerRow;
    int rows;
    bool isApproved;
    int tankCapacity;
    int currentFuel;
    bool isRefueling;
    int flightNumber;
    int TimeToTakeOffAndLandingInSeconds;
    vector<std::vector<bool>> seats;
    mutex seatMutex;
    int takeSeatTimeInMilliseconds = 100;
    chrono::steady_clock::time_point landingTime;
    chrono::steady_clock::time_point readyToTakeOffTime;
    chrono::steady_clock::time_point actualTakeOffTime;


    Airplane (int flightNumber, AirplaneType type)
    {
        switch (type)
        {
            case SMALL:
                rows = 4;
                seatsPerRow = 4;
                tankCapacity = 70;
                TimeToTakeOffAndLandingInSeconds = 2;
                break;
            case MEDIUM:
                rows = 8;
                seatsPerRow = 5;
                tankCapacity = 110;
                TimeToTakeOffAndLandingInSeconds = 3;
                break;
            case LARGE:
                rows = 10;
                seatsPerRow = 5;
                tankCapacity = 150;
                TimeToTakeOffAndLandingInSeconds = 4;
                break;
            case MAX:
                rows = 15;
                seatsPerRow = 6;
                tankCapacity = 220;
                TimeToTakeOffAndLandingInSeconds = 6;
                break;
        }

        this->flightNumber = flightNumber;
        currentFuel = 0;

        seats.resize(rows, vector<bool>(seatsPerRow, false));
    }

    bool tryTakeSeat(int familySize) {
        std::lock_guard lock(seatMutex);

        for (int row = 0; row < rows; ++row) {
            for (int seat = 0; seat <= seatsPerRow - familySize; ++seat) {
                bool allFree = true;

                // Sprawdź czy miejsca są wolne
                for (int i = 0; i < familySize; ++i) {
                    if (seats[row][seat + i]) {
                        allFree = false;
                        break;
                    }
                }

                // Jeśli wszystkie miejsca wolne – zajmij je
                if (allFree) {
                    for (int i = 0; i < familySize; ++i) {
                        seats[row][seat + i] = true;
                        this_thread::sleep_for(chrono::milliseconds(takeSeatTimeInMilliseconds));
                    }
                    return true;
                }
            }
        }

        // Brak wolnego ciągu miejsc
        return false;
    }


    vector<vector<bool>> getSeats() {
        std::lock_guard lock(seatMutex);
        return seats;
    }
};



#endif //AIRPLANE_H
