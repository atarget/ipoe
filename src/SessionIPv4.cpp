//
// Created by root on 03.09.19.
//

#include "SessionIPv4.h"
#include "Helper.h"

in_addr_t SessionIPv4::getDst_addr_v4() const {
    return dst_addr_v4;
}

void SessionIPv4::setDst_addr_v4(in_addr_t dst_addr_v4) {
    SessionIPv4::dst_addr_v4 = dst_addr_v4;
}

in_addr_t SessionIPv4::getSrc_addr_v4() const {
    return src_addr_v4;
}

void SessionIPv4::setSrc_addr_v4(in_addr_t src_addr_v4) {
    SessionIPv4::src_addr_v4 = src_addr_v4;
}

int SessionIPv4::getIf_index() const {
    return if_index;
}

void SessionIPv4::setIf_index(int if_index) {
    SessionIPv4::if_index = if_index;
}

bool SessionIPv4::isMaster() const {
    return master;
}

void SessionIPv4::setMaster(bool master) {
    SessionIPv4::master = master;
}

string SessionIPv4::ifName() {
    return Helper::if_indexToString(if_index);
}

void SessionIPv4::setIface(const string &ifname) {
    if_index = Helper::if_indexFromName(ifname);
}

string SessionIPv4::to_string() {
    return Helper::ipv4ToString(dst_addr_v4) + " src " + Helper::ipv4ToString(src_addr_v4) + " dev " + ifName();
}

string SessionIPv4::dstAsString() {
    return Helper::ipv4ToString(dst_addr_v4);
}

string SessionIPv4::srsAsString() {
    return Helper::ipv4ToString(src_addr_v4);
}
