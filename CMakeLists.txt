cmake_minimum_required(VERSION 3.10)
project(authentificator)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 14)
add_subdirectory(vendor/nlohmann_json)


add_executable(ipoe main.cpp
        src/Configuration.cpp src/Configuration.h
        src/SessionIPv4.cpp src/SessionIPv4.h
        src/Console.cpp src/Console.h
        src/Storage.cpp src/Storage.h
        src/InterfaceListener.cpp src/InterfaceListener.h
        src/NetlinkListener.cpp src/NetlinkListener.h
        src/Log.cpp src/Log.h src/StaticIPv4Route.cpp
        src/StaticIPv4Route.h src/IPv4AddressInfo.cpp
        src/IPv4AddressInfo.h src/Engine.cpp
        src/Engine.h src/Helper.cpp src/Helper.h src/Router.cpp src/Router.h)

add_executable(fake_arp_flooder fake_arp_flooder.cpp )

target_link_libraries(ipoe PRIVATE nlohmann_json::nlohmann_json pthread tins)
target_link_libraries(fake_arp_flooder PRIVATE pthread tins)