//
// Created by root on 03.09.19.
//

#include "InterfaceListener.h"
#include "Log.h"
#include "DHCP.h"
bool is_dhcp_sniffer_runned = false;
InterfaceListener::InterfaceListener(const uint32_t if_index, Storage *storage, Configuration *cfg, uint32_t arp_socket):
    if_index(if_index), storage(storage), cfg(cfg), arp_socket(arp_socket) {

    Log::info("Starting interface listener on " + Helper::if_indexToString(if_index));
    if_name = Helper::if_indexToString(if_index);
    iface_hw_addr = Helper::hwAddress(if_index);

    snifferThread = new thread(&InterfaceListener::snifferRoutine, this);
    if (!is_dhcp_sniffer_runned) {
        is_dhcp_sniffer_runned = true;
        snifferDHCPThread = new thread(&InterfaceListener::snifferDHCPRoutine, this);
    }
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

void InterfaceListener::snifferDHCPRoutine() {
    while (true) {
        try {
            SnifferConfiguration configDhcp;
            configDhcp.set_promisc_mode(false);
            configDhcp.set_direction(PCAP_D_IN);
            configDhcp.set_filter("udp and src port 67");
            Sniffer snifferDhcp(cfg->getRealyIface(), configDhcp);
            snifferDhcp.sniff_loop(std::bind(&InterfaceListener::callbackDHCP, this, std::placeholders::_1));
        } catch (exception e) {
            Log::warn("Сниффер просрал - пробуем еще раз" + string(e.what()));
            sleep(1);
        }
    }
}

bool InterfaceListener::callbackDHCP(const PDU &pdu) {
    return tryDhcpSession(pdu);
}
bool InterfaceListener::callback(const PDU &pdu) {
        if (tryIpSession(pdu)) return true;
        const ARP &arp = pdu.rfind_pdu<ARP>();
        const string hwAddr = arp.sender_hw_addr().to_string();
        this->proxyArp(arp);
        if (storage->isControlGateway(arp.sender_ip_addr())) {
            storage->addIPv4Clent(IPv4AddressInfo(arp.sender_ip_addr(), if_index), hwAddr, "");
        }
        return true;
}

bool InterfaceListener::tryDhcpSession(const PDU &pdu) {
    try {
        Log::debug("DHCP test");
        const UDP &udp = pdu.rfind_pdu<UDP>();
        auto raw = udp.inner_pdu()->serialize();
        if (raw.size() >= sizeof(DHCPHEADER)) {
            DHCPHEADER *header = (DHCPHEADER *) raw.data();
            if (header->op == DHCP::OpCodes::BOOTREPLY) {
                IPv4Address addr = IPv4Address(header->yiaddr);
                HWAddress<6> hwAddr = HWAddress<6>(header->chaddr);
                Log::debug("DHCP bind mac " + hwAddr.to_string() +
                    " to ip " + addr.to_string());
                storage->addIPv4Clent(IPv4AddressInfo(addr, 0), hwAddr.to_string(), hwAddr.to_string());
            }
        }
    } catch (exception &ex) {
         Log::debug("NOT DHCP " + string(ex.what()));
        return true;
    }
    return true;
}


bool InterfaceListener::tryIpSession(const PDU &pdu) {
    try {
        const IP& ip = pdu.rfind_pdu<IP>();
        const EthernetII &frame = pdu.rfind_pdu<EthernetII>();
        if (storage->isControlGateway(ip.src_addr())) {
            storage->addIPv4Clent(IPv4AddressInfo(ip.src_addr(), if_index), frame.src_addr().to_string(), "");
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

