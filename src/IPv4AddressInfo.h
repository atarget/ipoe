//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_IPV4ADDRESSINFO_H
#define AUTHENTIFICATOR_IPV4ADDRESSINFO_H


#include <stdint-gcc.h>
#include <netinet/in.h>
#include <string>
#include "Helper.h"

using namespace std;

class IPv4AddressInfo {
public:
    IPv4AddressInfo();
    IPv4AddressInfo(IPv4Address, uint32_t);
    void setIf_index(uint32_t if_index);
    void setMask(uint8_t mask);
    void setAddress(in_addr_t address);

    in_addr_t getAddress() const;
    string to_string() const;
    uint32_t getIf_index() const;
    uint8_t getMask() const;
    uint8_t getBitmask() const;

    string bitmaskAsString() const;
    string interfaceName() const;

    in_addr_t getNetwork() const;

    void setIsGw(bool val);
    bool operator == (in_addr_t addr);
    bool operator == (IPv4AddressInfo addr);

    string stringNetwork();
private:
    bool isGw = false;
    uint32_t if_index;
    uint8_t mask;
    in_addr_t address;
    in_addr_t network;
    in_addr_t bcast;
    uint32_t bitmask;
};


#endif //AUTHENTIFICATOR_IPV4ADDRESSINFO_H
