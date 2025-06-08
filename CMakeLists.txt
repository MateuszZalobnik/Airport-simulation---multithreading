cmake_minimum_required(VERSION 3.29)
project(SO2_Lot)

set(CMAKE_CXX_STANDARD 20)

add_executable(SO2_Lot main.cpp
        Airplane/Airplane.h
        Airport/Airport.cpp
        Airport/Airport.h
        AirportService/Tanker.h
        AirportService/Tanker.cpp)

target_link_libraries(SO2_Lot
        PRIVATE
        sfml-audio
)
