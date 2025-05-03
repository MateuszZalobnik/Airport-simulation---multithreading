#ifndef AIRPLANE_H
#define AIRPLANE_H

enum AirplaneType
{
    SMALL = 0,
    MEDIUM = 1,
    LARGE = 2,
    MAX = 3,
};

class Airplane {
protected:
    int rows;
    int seatsPerRow;
private:
public:
    bool isApproved;
    int tankCapacity;
    int currentFuel;
    bool isRefueling;
    int flightNumber;
    int TimeToTakeOffAndLandingInSeconds;

    Airplane (int flightNumber, AirplaneType type)
    {
        switch (type)
        {
            case SMALL:
                rows = 10;
                seatsPerRow = 4;
                tankCapacity = 70;
                TimeToTakeOffAndLandingInSeconds = 2;
                break;
            case MEDIUM:
                rows = 20;
                seatsPerRow = 6;
                tankCapacity = 110;
                TimeToTakeOffAndLandingInSeconds = 3;
                break;
            case LARGE:
                rows = 30;
                seatsPerRow = 8;
                tankCapacity = 150;
                TimeToTakeOffAndLandingInSeconds = 4;
                break;
            case MAX:
                rows = 40;
                seatsPerRow = 8;
                tankCapacity = 220;
                TimeToTakeOffAndLandingInSeconds = 6;
                break;
        }

        this->flightNumber = flightNumber;
        currentFuel = 0;
    }
};



#endif //AIRPLANE_H
