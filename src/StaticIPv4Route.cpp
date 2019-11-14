//
// Created by root on 03.09.19.
//

#include "StaticIPv4Route.h"
#include "Helper.h"

void StaticIPv4Route::setNetwork(uint32_t network) {
    this->network = network;
    bcast = Helper::makeIPv4Bcast(network, mask);
}

void StaticIPv4Route::setVia(uint32_t via) {
    this->via = via;
}

void StaticIPv4Route::setMask(uint8_t mask) {
    this->mask = mask;
    bitmask = Helper::makeIPv4Bitmask(mask);
    bcast = Helper::makeIPv4Bcast(network, mask);
}

void StaticIPv4Route::setSrc(uint32_t src) {
    this->src = src;
}

void StaticIPv4Route::setIfIndex(uint32_t if_index) {
    this->if_index = if_index;
}

in_addr_t StaticIPv4Route::getNetwork() const {
    return network;
}

uint8_t StaticIPv4Route::getMask() const {
    return mask;
}

in_addr_t StaticIPv4Route::getBcast() const {
    return bcast;
}

in_addr_t StaticIPv4Route::getVia() const {
    return via;
}

in_addr_t StaticIPv4Route::getSrc() const {
    return src;
}

uint32_t StaticIPv4Route::getIf_index() const {
    return if_index;
}

string StaticIPv4Route::interfaceName() const {
    return Helper::if_indexToString(if_index);
}

void StaticIPv4Route::setNetwork(string network) {
    this->network = Helper::ipv4FromString(network);
}

void StaticIPv4Route::setVia(string via) {
    this->via = Helper::ipv4FromString(via);
}

void StaticIPv4Route::setSrc(string src) {
    this->src = Helper::ipv4FromString(src);
}

void StaticIPv4Route::setInterface(string name) {
    if_index = Helper::if_indexFromName(name);
}

string StaticIPv4Route::stringNetwork() const {
    return Helper::ipv4ToString(network);
}

string StaticIPv4Route::stringSrc() const {
    return Helper::ipv4ToString(src);
}

string StaticIPv4Route::stringVia() const {
    return Helper::ipv4ToString(via);
}

string StaticIPv4Route::stringBcast() const {
    return Helper::ipv4ToString(bcast);
}

void StaticIPv4Route::install() {
    this->installed = true;
}

void StaticIPv4Route::uninstall() {
    this->installed = false;
}

bool StaticIPv4Route::isInstalled() {
    return installed;
}

void StaticIPv4Route::setPrefix(string prefix) {
    auto parts = Helper::explode(prefix, '/');
    setNetwork(parts.at(0));
    setMask(atoi(parts.at(1).c_str()));
}

string StaticIPv4Route::stringPrefix() const {
    return stringNetwork() + "/" + std::to_string(mask);
}

const string StaticIPv4Route::to_string() {
    return stringPrefix() + " dev " + interfaceName() + " via " +
        Helper::ipv4ToString(via) + " src " + Helper::ipv4ToString(src);
}

bool StaticIPv4Route::isInNetwork(in_addr_t addr) {
    in_addr_t  addr_network = Helper::makeNetworkAddress(addr, mask);
    return network == addr_network;
}
