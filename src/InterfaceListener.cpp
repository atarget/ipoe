//
// Created by root on 03.09.19.
//

#include "InterfaceListener.h"
#include "Log.h"

InterfaceListener::InterfaceListener(const uint32_t if_index, Storage *storage, Configuration *cfg, uint32_t arp_socket):
    if_index(if_index), storage(storage), cfg(cfg), arp_socket(arp_socket) {
    Log::info("Starting interface listener on " + Helper::if_indexToString(if_index));
    if_name = Helper::if_indexToString(if_index);
    iface_hw_addr = Helper::hwAddress(if_index);

    snifferThread = new thread(&InterfaceListener::snifferRoutine, this);
    h_proxyArpThreadRoutine = new thread(&InterfaceListener::proxyArpThreadRoutine, this);
    h_proxyArpAntiDDOSTimer = new thread(&InterfaceListener::proxyArpAntiDDOSRoutine, this);
}

void InterfaceListener::snifferRoutine() {
    while (true) {
        try {
            SnifferConfiguration config;
            config.set_promisc_mode(false);
            config.set_direction(PCAP_D_IN);
            config.set_filter(cfg->getBpf());
            Sniffer sniffer(Helper::if_indexToString(if_index), config);
            sniffer.sniff_loop(std::bind(&InterfaceListener::callback, this, std::placeholders::_1));
        } catch (exception e) {
            Log::warn("Сниффер просрал - пробуем еще раз");
            sleep(1);
        }
    }
}

bool InterfaceListener::callback(const PDU &pdu) {
        if (tryIpSession(pdu)) return true;
        const ARP &arp = pdu.rfind_pdu<ARP>();
        this->proxyArp(arp);
        if (storage->isControlGateway(arp.sender_ip_addr())) {
            storage->addIPv4Clent(IPv4AddressInfo(arp.sender_ip_addr(), if_index));
        }
        return true;
}

bool InterfaceListener::tryIpSession(const PDU &pdu) {
    try {
        const IP& ip = pdu.rfind_pdu<IP>();
        if (storage->isControlGateway(ip.src_addr())) {
            storage->addIPv4Clent(IPv4AddressInfo(ip.src_addr(), if_index));
        }
    } catch (exception &ex) {
        return  false;
    }
    return true;
}

void InterfaceListener::proxyArp(const ARP &arp) {
    if (arp.opcode() == ARP::REPLY) return;
    if (!storage->isControlGateway(arp.target_ip_addr())) return;
    if (arp.target_ip_addr().to_string() == arp.sender_ip_addr().to_string()) {
        // client do garp
        return;
    }


    if (globalProxyArpDDOSCounter > 100) {
        Log::warn("ARP global DDoS prevent");
        return;
    }

    proxyArpAntiDDOSCountersMutex.lock();
    if (cfg->getSkipProxyArp().count(arp.sender_ip_addr())) {
        proxyArpAntiDDOSCountersMutex.unlock();
//        Log::debug("Skip proxy arp for " + arp.sender_ip_addr().to_string());
        return;
    }
    proxyArpAntiDDOSCounters[arp.sender_ip_addr().to_string()]++;
    if (proxyArpAntiDDOSCounters[arp.sender_ip_addr().to_string()] > 15) {
        proxyArpAntiDDOSCountersMutex.unlock();
        Log::warn("ARP DDoS prevent from " + arp.sender_ip_addr().to_string());
        return;
    }
    globalProxyArpDDOSCounter++;
    proxyArpAntiDDOSCountersMutex.unlock();


    if (storage->isControlGateway(arp.target_ip_addr()) &&
        storage->isSessionMaster(arp.target_ip_addr()) &&
        storage->isSessionMaster(arp.sender_ip_addr())) {
        struct EthernetII reply = ARP::make_arp_reply(
                arp.sender_ip_addr(),
                arp.target_ip_addr(),
                arp.sender_hw_addr(),
                iface_hw_addr);
        proxyArpMtx.lock();
        arpProxyMap[arp.sender_ip_addr().to_string()] = reply;
        proxyArpMtx.unlock();
        proxyArpMtxCv.notify_one();
    }

}

void InterfaceListener::proxyArpThreadRoutine() {
    struct sockaddr sa;
    strcpy(sa.sa_data, if_name.c_str());
    EthernetII reply;
    while (1) {
        unique_lock<mutex> lk(proxyArpMtxWait);
        while (arpProxyMap.empty()) proxyArpMtxCv.wait(lk);
        proxyArpMtx.lock();
        map<string, EthernetII>::iterator it;
        for (it = arpProxyMap.begin(); it != arpProxyMap.end(); it++) {
            reply = it->second;
            auto buf = reply.serialize();
            Log::debug("Proxy arp " + it->first);
            sendto(arp_socket, buf.data(), buf.size(), 0, &sa, sizeof(sa));
        }
        arpProxyMap.clear();
        proxyArpMtx.unlock();
    }
}

void InterfaceListener::proxyArpAntiDDOSRoutine() {
    while (1) {
        proxyArpAntiDDOSCountersMutex.lock();
        proxyArpAntiDDOSCounters.clear();
        globalProxyArpDDOSCounter = 0;
        proxyArpAntiDDOSCountersMutex.unlock();
        this_thread::sleep_for(chrono::seconds(1));
    }
}
