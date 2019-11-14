//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_INTERFACELISTENER_H
#define AUTHENTIFICATOR_INTERFACELISTENER_H


#include <cstdint>
#include <condition_variable>
#include "Storage.h"

class InterfaceListener {
public:
    InterfaceListener(const uint32_t if_index, Storage *storage, Configuration *cfg, uint32_t arp_socket);

private:
    uint32_t if_index;
    string if_name;
    Storage *storage;
    Configuration *cfg;

    thread *snifferThread;
    uint32_t arp_socket;
    void snifferRoutine();

    HWAddress<6> iface_hw_addr;
    bool callback(const PDU &pdu);
    bool tryIpSession(const PDU &pdu);
    void proxyArp(const ARP &arp);
    map<string, EthernetII> arpProxyMap;
    int globalProxyArpDDOSCounter;
    mutex proxyArpAntiDDOSCountersMutex;
    map<string, uint32_t> proxyArpAntiDDOSCounters;
    mutex proxyArpMtx;
    mutex proxyArpMtxWait;
    condition_variable proxyArpMtxCv;
    void proxyArpAntiDDOSRoutine();
    void proxyArpThreadRoutine();
    thread *h_proxyArpThreadRoutine;
    thread *h_proxyArpAntiDDOSTimer;
};


#endif //AUTHENTIFICATOR_INTERFACELISTENER_H
