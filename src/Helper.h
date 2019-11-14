//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_HELPER_H
#define AUTHENTIFICATOR_HELPER_H


#include <cstdint>
#include <tins/tins.h>

using namespace std;
using namespace Tins;

class Helper {
public:
    static uint32_t makeIPv4Bitmask(uint8_t mask);
    static in_addr_t makeIPv4Bcast(in_addr_t address, uint8_t mask);
    static string if_indexToString(uint32_t if_index);
    static string ipv4ToString(in_addr_t addr);
    static in_addr_t ipv4FromString(string addr);
    static uint32_t if_indexFromName(string name);
    static in_addr_t makeNetworkAddress(in_addr_t addr, uint8_t mask);
    static std::vector<std::string> explode(std::string const & s, char delim);
    static std::string exec(const string &cmd);
    static HWAddress<6> hwAddress(const string &ifname);
    static HWAddress<6> hwAddress(const uint32_t if_index);
};


#endif //AUTHENTIFICATOR_HELPER_H
