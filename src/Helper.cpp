//
// Created by root on 03.09.19.
//

#include <tgmath.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Helper.h"
#include "Log.h"

uint32_t Helper::makeIPv4Bitmask(uint8_t mask) {
    uint32_t bitmask = pow(2,  (32 - mask)) - 1;
    bitmask = ~bitmask;
    return bitmask;
    /*
    uint32_t reversedBitmask = 0;
    for (int i = 0; i < 32; i++) {
        uint32_t currentBitInNormal = (bitmask << (31 - i));
        currentBitInNormal >>= 31;
        currentBitInNormal <<= 31 - i;
        reversedBitmask |= currentBitInNormal;
    }
    return reversedBitmask;
     */
}

in_addr_t Helper::makeIPv4Bcast(in_addr_t address, uint8_t mask) {
    uint32_t invertedBitmask = ~makeIPv4Bitmask(mask);
    return ntohl(htonl(address) | invertedBitmask);
}

string Helper::if_indexToString(uint32_t if_index) {
    try {
        NetworkInterface iface = NetworkInterface::from_index(if_index);
        return iface.name();
    } catch (exception e) {
        return "unknown";
    }
}

string Helper::ipv4ToString(in_addr_t addr) {
    char tmp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, tmp, INET_ADDRSTRLEN);
    return tmp;
}

in_addr_t Helper::ipv4FromString(string addr) {
    in_addr_t result;
    inet_pton(AF_INET, addr.c_str(), &result);
    return result;
}

uint32_t Helper::if_indexFromName(string name) {
    try {
        NetworkInterface iface(name);
        return iface.id();
    } catch (exception e) {
        return 0;
    }
}

in_addr_t Helper::makeNetworkAddress(in_addr_t addr, uint8_t mask) {
    in_addr_t bitmask = makeIPv4Bitmask(mask);
    return  ntohl(htonl(addr) & bitmask);
}

std::vector<std::string> Helper::explode(std::string const & s, char delim) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    for (std::string token; std::getline(iss, token, delim); ) {
        result.push_back(std::move(token));
    }
    return result;
}

std::string Helper::exec(const string &cmd) {
    char buffer[2048];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

HWAddress<6> Helper::hwAddress(const string &ifname) {
    NetworkInterface iface(ifname);
    return iface.hw_address();
}

HWAddress<6> Helper::hwAddress(const uint32_t if_index) {
    NetworkInterface iface = NetworkInterface::from_index(if_index);
    return iface.hw_address();
}
