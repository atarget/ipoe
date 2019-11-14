//
// Created by root on 03.09.19.
//


#include <strings.h>
#include <arpa/inet.h>
#include <bitset>
#include <iostream>
#include <tgmath.h>
#include "IPv4AddressInfo.h"
#include "Helper.h"

void IPv4AddressInfo::setIf_index(uint32_t if_index) {
    IPv4AddressInfo::if_index = if_index;
}

void IPv4AddressInfo::setMask(uint8_t mask) {
    IPv4AddressInfo::mask = mask;
    bitmask = Helper::makeIPv4Bitmask(mask);
    network = Helper::makeNetworkAddress(address, mask);
    bcast = Helper::makeIPv4Bcast(network, mask);
}

void IPv4AddressInfo::setAddress(in_addr_t address) {
    IPv4AddressInfo::address = address;
    network = Helper::makeNetworkAddress(address, mask);
    bcast = Helper::makeIPv4Bcast(network, mask);
}

string IPv4AddressInfo::to_string() const {
    return Helper::ipv4ToString(address);
}

in_addr_t IPv4AddressInfo::getAddress() const {
    return address;
}

uint32_t IPv4AddressInfo::getIf_index() const {
    return if_index;
}

uint8_t IPv4AddressInfo::getMask() const {
    return mask;
}

uint8_t IPv4AddressInfo::getBitmask() const {
    return bitmask;
}

string IPv4AddressInfo::bitmaskAsString() const {
    return std::bitset<32>(IPv4AddressInfo::bitmask).to_string();
}

string IPv4AddressInfo::interfaceName() const {
    return Helper::if_indexToString(if_index);
}

in_addr_t IPv4AddressInfo::getNetwork() const {
    return network;
}

void IPv4AddressInfo::setIsGw(bool val) {
    isGw = val;
}

bool IPv4AddressInfo::operator==(in_addr_t addr) {
    if (!isGw) return addr == address;
    int net = Helper::makeNetworkAddress(addr, mask);
    return net == network;
}

bool IPv4AddressInfo::operator==(IPv4AddressInfo addr) {
    return (addr.address == address) && (addr.mask == addr.mask);
}

string IPv4AddressInfo::stringNetwork() {
    return Helper::ipv4ToString(getNetwork());
}

IPv4AddressInfo::IPv4AddressInfo(IPv4Address fromAddress, uint32_t if_index) {
    this->if_index = if_index;
    this->setAddress(fromAddress);
    this->setMask(32);
}

IPv4AddressInfo::IPv4AddressInfo() {

}
