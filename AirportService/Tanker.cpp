#include "Tanker.h"
#include "../Airport/Airport.h"  // pełna definicja Airport tutaj

void Tanker::run(Airport* airport, int tankTimeInMilliseconds)
{
    while (true)
    {
        Airplane* target = nullptr;

        {
            lock_guard<mutex> lock(airport->queueMutex);
            for (auto& gate : airport->gates)
            {
                auto airplane = gate.get()->airplane;
                if (airplane && airplane->currentFuel < airplane->tankCapacity && !airplane->isRefueling)
                {
                    airplane->isRefueling = true;
                    target = airplane;
                    break;
                }
            }
        }

        if (target)
        {
            while (target->currentFuel < target->tankCapacity)
            {
                if (this->fuelLevel <= 0)
                {
                    // zwalniamy samolot i pozwalamy innym tankować
                    this->refill();
                    break;
                }

                // Tankowanie: 10 jednostek na raz
                int amount = min(10, min(this->fuelLevel, target->tankCapacity - target->currentFuel));
                this->fuelLevel -= amount;
                target->currentFuel += amount;

                this_thread::sleep_for(chrono::milliseconds(tankTimeInMilliseconds));
            }


            {
                lock_guard<mutex> lock(airport->queueMutex);
                target->isRefueling = false;
                target = nullptr;
            }
        }
        else
        {
            // Brak samolotów do tankowania – czekaj chwilę
            this_thread::sleep_for(chrono::milliseconds(500));
        }
    }
}