//
// Created by root on 03.09.19.
//

#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <regex>
#include "Storage.h"
#include "Helper.h"
#include "Log.h"

Storage::Storage(Configuration *cfg, Router *router) {
    this->cfg = cfg;
    this->router = router;
    routerCommanderStaticRoutesThread = new thread(&Storage::routerCommanderRoutine_StaticRoutesV4, this);
    routerCommanderSessionsThread = new thread(&Storage::routerCommanderRoutine_SessionsV4, this);
    garpThread = new thread(&Storage::garpThreadRoutine, this);
    readStaticRoutes();
    cacheThread = new thread(&Storage::cacheRoutine, this);
    controlGwStartThread = new thread(&Storage::controlGwStart, this);
    savingThread = new thread(&Storage::savingThreadRoutine, this);
}

void Storage::controlGwStart() {
    string currentAddresses = Helper::exec("ip ad sh dev " + cfg->getControlInterface());
    regex  expr("inet (\\d+\\.\\d+\\.\\d+\\.\\d+)\\/(\\d+)");
    smatch m;
    while (regex_search (currentAddresses, m, expr)) {
        string tmp = m[1];
        IPv4Address address(tmp);
        tmp = m[2];
        IPv4AddressInfo gw;
        gw.setMask(atoi(tmp.c_str()));
        gw.setAddress(address);
        gw.setIf_index(Helper::if_indexFromName(cfg->getControlInterface()));
        addControlGateway(gw);
        currentAddresses = m.suffix().str();
    }
}

void Storage::addIPv4Clent(IPv4AddressInfo address, string hwAddr, string realHwAddr) {
    /*
     * 1. Search session in sessions
     * 1.1 If not found create it
     * 2. Search gw in controlled gateways
     * 3. Set session master flag
     * 4. If session master flag changed - put it to routerSessionsIPv4Queue
     * 5. set session in storage
     *
     */

    if (address.getAddress() == 0) return;
    ipv4staticRoutesMutex.lock();
    for (auto staticRoutesOfClient: ipv4StaticRoutes) {
        for (auto route: staticRoutesOfClient.second) {
            StaticIPv4Route rt = route.second;
            if (rt.isInNetwork(address.getAddress())) {
                ipv4staticRoutesMutex.unlock();
                return;
            }
        }
    }
    ipv4staticRoutesMutex.unlock();
    ipv4GwsMutex.lock();
    for (auto gw: ipv4Gws) {
        if (gw.getAddress() == address.getAddress()) { // it is gw - drop it
            ipv4GwsMutex.unlock();
            return;
        }
    }
    ipv4GwsMutex.unlock();
    SessionIPv4 session;
    bool if_index_changed = false;
    ipv4SessionsMutex.lock();
    if (ipv4Sessions.count(address.getAddress())) {
        session = ipv4Sessions[address.getAddress()];
        if (session.getIf_index() != address.getIf_index() && address.getIf_index()) {
            if_index_changed = true;
        }
    }
    ipv4SessionsMutex.unlock();
    if (address.getIf_index()) {
        if (session.getRealHwAddr() == "" || session.getRealHwAddr() == hwAddr) {
            session.setIf_index(address.getIf_index());
        }
    }
    session.setDst_addr_v4(address.getAddress());
    if (realHwAddr != "") {
        session.setRealHwAddr(realHwAddr);
    }
    session.setHwAddr(hwAddr);
    bool masterFlagSaved = session.isMaster();
    if (isControlGateway(address.getAddress())) {
        IPv4AddressInfo gw = getControlGwFor(address.getAddress());
        session.setSrc_addr_v4(gw.getAddress());
        session.setMaster(true);
    } else {
        session.setMaster(false);
    }
    ipv4SessionsMutex.lock();
    ipv4Sessions[address.getAddress()] = session;
    ipv4SessionsMutex.unlock();
    if (session.getIf_index())
    if (masterFlagSaved != session.isMaster() || if_index_changed ) {
        pushSessionToRouterCommander(session);
        cacheSession(session);
    }
}

void Storage::addControlGateway(IPv4AddressInfo gateway) {

    /**
     * 1. Replace control gateway on storage
     * 2. Find all sessions for this gw
     * 3. iterate it and if master flag changed put it to routerSessionsIPv4Queue
     *
     */
    gateway.setIsGw(true);
    ipv4GwsMutex.lock();
    int cnt = std::count(ipv4Gws.begin(), ipv4Gws.end(), gateway);
    if (!cnt) {
        ipv4Gws.push_back(gateway);
    }
    ipv4GwsMutex.unlock();

    ipv4SessionsMutex.lock();
    for (auto sessionPair : ipv4Sessions) {
        auto addr = sessionPair.first;
        auto session = sessionPair.second;
        /** == overrided in IPv4AddressInfo */
        if (gateway == addr) {
            if (!session.isMaster()) {
                session.setMaster(true);
                ipv4Sessions[addr] = session;
                pushSessionToRouterCommander(session);
            }
        }
    }
    Log::info("Control gw added: " + gateway.to_string() + "/" + to_string(gateway.getMask()));
    ipv4SessionsMutex.unlock();
    string arptablesCmd = "arptables -D OUTPUT -d " + gateway.stringNetwork() + "/" + to_string(gateway.getMask())
            + " -j mangle --mangle-ip-s " + Helper::ipv4ToString(gateway.getAddress());
    system(arptablesCmd.c_str());
    arptablesCmd = "arptables -A OUTPUT -d " + gateway.stringNetwork() + "/" + to_string(gateway.getMask())
                   + " -j mangle --mangle-ip-s " + Helper::ipv4ToString(gateway.getAddress());
    system(arptablesCmd.c_str());

    garpQueueMutex.lock();
    garpQueue.push_back(gateway);
    garpQueueMutex.unlock();
    garpLockCv.notify_one();
}

void Storage::delControlGateway(IPv4AddressInfo gateway) {
    /**
     * 1. Remove control gateway from storage
     * 2. Find all sessions for this gw
     * 3. iterate it and if master flag changed put it to routerSessionsIPv4Queue
     *
     */
    ipv4GwsMutex.lock();
    for (auto itr = ipv4Gws.begin(); itr != ipv4Gws.end(); itr++) {
        if (itr->getAddress() == gateway.getAddress()) {
            ipv4Gws.erase(itr);
            break;
        }
    }
    ipv4GwsMutex.unlock();
    gateway.setIsGw(true);
    ipv4SessionsMutex.lock();
    for (auto sessionPair : ipv4Sessions) {
        auto addr = sessionPair.first;
        auto session = sessionPair.second;
        /** == overrided in IPv4AddressInfo */
        if (gateway == addr) {
            if (session.isMaster()) {
                session.setMaster(false);
                ipv4Sessions[addr] = session;
                pushSessionToRouterCommander(session);
            }
        }
    }
    Log::info("Control gw removed: " + gateway.to_string() + "/" + to_string(gateway.getMask()));
    ipv4SessionsMutex.unlock();
    string arptablesCmd = "arptables -D OUTPUT -d " + gateway.stringNetwork() + "/" + to_string(gateway.getMask())
                          + " -j mangle --mangle-ip-s " + Helper::ipv4ToString(gateway.getAddress());
    system(arptablesCmd.c_str());


}



IPv4AddressInfo Storage::getControlGwFor(in_addr_t addr) {
    if (isControlGateway(addr)) {
        ipv4GwsMutex.lock();
        auto itr = std::find(ipv4Gws.begin(), ipv4Gws.end(), addr);
        IPv4AddressInfo result = *itr;
        ipv4GwsMutex.unlock();
        return result;
    }
    return IPv4AddressInfo();
}

bool Storage::isControlGateway(in_addr_t addr) {
    ipv4GwsMutex.lock();
    auto itr = std::find(ipv4Gws.begin(), ipv4Gws.end(), addr);
    bool result = (itr != ipv4Gws.end());
    ipv4GwsMutex.unlock();
    return result;
}

void Storage::routeAdded(StaticIPv4Route route) {
    /**
     * 1. find target in sessions
     * 2. if it finded and it master
     * 2.1 find all static routes for this address
     * 2.2 set static route flag installed
     * 2.3 put it to routerStaticRoutesIPv4Queue
     * 3. if it finded and it slave or not finded
     * 3.1 find all static routes for this address
     * 3.2 set static route flag uninstalled
     * 3.3 put it to routerStaticRoutesIPv4Queue
     */
     ipv4SessionsMutex.lock();
     int sessionsCnt = ipv4Sessions.count(route.getNetwork());
     if (!sessionsCnt) {
         ipv4SessionsMutex.unlock();
         processStaticRoutesOf(route.getNetwork(), false);
         return;
     }
     SessionIPv4 session = ipv4Sessions[route.getNetwork()];
     ipv4SessionsMutex.unlock();
     if (session.isMaster()) {
         processStaticRoutesOf(route.getNetwork(), true);
     } else {
         processStaticRoutesOf(route.getNetwork(), false);
     }
}

void Storage::routeDeleted(StaticIPv4Route route) {
    /**
     * 1. find target in sessions
     * 2. if it finded and it master
     * 2.1. put it to routerSessionsIPv4Queue
     * 3. if it finded and it slave or not found
     * 3.1 find all static routes for this address
     * 3.2 set static route flag installed
     * 3.3 put it to routerStaticRoutesIPv4Queue
     */
    ipv4SessionsMutex.lock();
    int sessionsCnt = ipv4Sessions.count(route.getNetwork());
    if (!sessionsCnt) {
        ipv4SessionsMutex.unlock();
        processStaticRoutesOf(route.getNetwork(), false);
        return;
    }
    SessionIPv4 session = ipv4Sessions[route.getNetwork()];
    ipv4SessionsMutex.unlock();
    if (session.isMaster()) {
        session.setMaster(false);
        pushSessionToRouterCommander(session);
        processStaticRoutesOf(route.getNetwork(), false);
    } else {
        processStaticRoutesOf(route.getNetwork(), false);
    }

    ipv4SessionsMutex.lock();
    if (sessionsCnt) {
        ipv4Sessions.erase(route.getNetwork());
    }
    ipv4SessionsMutex.unlock();
}

void Storage::processStaticRoutesOf(in_addr_t addr, bool install) {
    ipv4staticRoutesMutex.lock();
    if (!ipv4StaticRoutes.count(addr)) {
        ipv4staticRoutesMutex.unlock();
        return;
    }
    auto routes = ipv4StaticRoutes[addr];
    for (auto routePair : routes) {
        auto route = routePair.second;
        StaticIPv4Route rt = route;
        if (install) route.install(); else route.uninstall();
        if (!isControlGateway(addr)) {
            route.uninstall();
        } else {
            auto gw = getControlGwFor(addr);
            route.setSrc(gw.getAddress());
        }
        pushStaticRouteToRouterCommander(route);
    }
    ipv4staticRoutesMutex.unlock();
}

void Storage::routerCommanderRoutine_StaticRoutesV4() {
    while (1) {
        routerStaticRoutesIPv4QueueMutex.lock();
        for (auto route: routerStaticRoutesIPv4Queue) {
            router->processIPv4StaticRoute(route);
        }
        routerStaticRoutesIPv4Queue.clear();
        routerStaticRoutesIPv4QueueMutex.unlock();
        unique_lock<mutex> lk(routerStaticRoutesIPv4QueueLock);
        while (routerStaticRoutesIPv4Queue.empty()) routerStaticRoutesIPv4QueueLockCv.wait(lk);
    }
}

void Storage::pushStaticRouteToRouterCommander(StaticIPv4Route route) {
    if (route.isInstalled()) {
        ipv4SessionsMutex.lock();
        if (!ipv4Sessions.count(route.getVia())) {
            Log::warn("Try add route with out via session - skip");
            ipv4SessionsMutex.unlock();
            return;
        }
        SessionIPv4 session = ipv4Sessions[route.getVia()];
        ipv4SessionsMutex.unlock();
        route.setIfIndex(session.getIf_index());
    }
    routerStaticRoutesIPv4QueueMutex.lock();
    routerStaticRoutesIPv4Queue.push_back(route);
    routerStaticRoutesIPv4QueueMutex.unlock();
    routerStaticRoutesIPv4QueueLockCv.notify_one();
}

void Storage::pushSessionToRouterCommander(SessionIPv4 session) {
    routerSessionsIPv4QueueMutex.lock();
    routerSessionsIPv4Queue.push_back(session);
    routerSessionsIPv4QueueMutex.unlock();
    routerSessionsIPv4QueueLockCv.notify_one();
}

void Storage::routerCommanderRoutine_SessionsV4() {
    while (1) {
        routerSessionsIPv4QueueMutex.lock();
        for (auto session: routerSessionsIPv4Queue) {
            router->processIPv4Session(session);
        }
        routerSessionsIPv4Queue.clear();
        routerSessionsIPv4QueueMutex.unlock();
        unique_lock<mutex> lk(routerSessionsIPv4QueueLock);
        while (routerSessionsIPv4Queue.empty()) routerSessionsIPv4QueueLockCv.wait(lk);
    }
}

void Storage::requeueAllSessions() {
    ipv4SessionsMutex.lock();
    for (auto sessionPair: ipv4Sessions) {
        auto addr = sessionPair.first;
        auto session = sessionPair.second;
        if (isControlGateway(addr)) {
            session.setMaster(true);
        } else {
            session.setMaster(false);
        }
        ipv4Sessions[addr] = session;
        ipv4SessionsMutex.unlock();
        pushSessionToRouterCommander(session);
        cacheSession(session);
        ipv4SessionsMutex.lock();
    }
    ipv4SessionsMutex.unlock();
}

void Storage::save() {
    ipv4SessionsMutex.lock();
    try {
        json j = json::array();
        for (auto sessionPair: ipv4Sessions) {
            auto addr = sessionPair.first;
            if (!addr) continue;
            auto session = sessionPair.second;
            json jSession = json::object();
            jSession["dst_addr"] = (uint32_t) session.getDst_addr_v4();
            jSession["src_addr"] = session.getSrc_addr_v4();
            jSession["if_name"] = session.ifName();
            jSession["master"] = session.isMaster();
            jSession["addr"] = addr;
            j.push_back(jSession);
        }
        ofstream f(cfg->getCacheFileName());
        f << j.dump();
        f.close();
    } catch (exception e) {
        Log::warn("Can not save cache");
    }
    ipv4SessionsMutex.unlock();
}

void Storage::load() {
    ipv4SessionsMutex.lock();
    try {
        json j;
        ifstream f(cfg->getCacheFileName());
        if (!f.is_open()) {
            ipv4SessionsMutex.unlock();
            return;
        }
        f >> j;
        f.close();
        for (auto s: j) {
            SessionIPv4 session;
            session.setIf_index(Helper::if_indexFromName(s["if_name"]));
            session.setDst_addr_v4(s["dst_addr"]);
            session.setSrc_addr_v4(s["src_addr"]);
            session.setMaster(s["master"]);
            ipv4Sessions[s["addr"]] = session;
            pushSessionToRouterCommander(session);
        }
    } catch (exception e) {
        Log::warn("Can not load cache - removind file");
        try {
            unlink(cfg->getCacheFileName().c_str());
        } catch (exception e) {

        }
    }
    ipv4SessionsMutex.unlock();
}


void Storage::requeueAllStaticRoutes() {
    ipv4staticRoutesMutex.lock();
    for (auto staticRoutesPair: ipv4StaticRoutes) {
        auto routes = staticRoutesPair.second;
        for (auto routePair: routes) {
            auto route = routePair.second;
            if (isControlGateway(route.getVia())) {
                route.install();
                IPv4AddressInfo gw = getControlGwFor(route.getVia());
                route.setSrc(gw.getAddress());
            } else {
                route.uninstall();
            }
            pushStaticRouteToRouterCommander(route);
        }
    }
    ipv4staticRoutesMutex.unlock();
}

void Storage::cacheRoutine() {
    struct sockaddr_in servaddr, cliaddr;
    inet_pton(AF_INET, cfg->getCacheBind().c_str(), &servaddr.sin_addr.s_addr);
    uint32_t trueflag = 1;
    cache_sosk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(cache_sosk, SOL_SOCKET, SO_BROADCAST, &trueflag, sizeof(trueflag));
    setsockopt(cache_sosk, SOL_SOCKET, SO_REUSEADDR, &trueflag, sizeof(trueflag));
    setsockopt(cache_sosk, SOL_SOCKET, SO_BINDTODEVICE, cfg->getControlInterface().c_str(), cfg->getControlInterface().length() + 1);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(cfg->getCachePort());
    while ( bind(cache_sosk, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) {
        Log::warn("Can not start cache client - retry in 1 second");
        close(cache_sosk);
        cache_sosk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        setsockopt(cache_sosk, SOL_SOCKET, SO_BROADCAST, &trueflag, sizeof(trueflag));
        setsockopt(cache_sosk, SOL_SOCKET, SO_REUSEADDR, &trueflag, sizeof(trueflag));
        setsockopt(cache_sosk, SOL_SOCKET, SO_BINDTODEVICE, cfg->getControlInterface().c_str(), cfg->getControlInterface().length() + 1);
        sleep(1);
    }
    Log::info("Cache client started");
    for (auto session: ipv4Sessions) {
        cacheSession(session.second);
    }
    Log::info("Full cache sended");


    while (1) {
        try {
            SnifferConfiguration config;
            config.set_direction(PCAP_D_IN);
            config.set_filter("udp and dst port " + to_string(cfg->getCachePort()));
            Sniffer sniffer(cfg->getControlInterface(), config);
            sniffer.sniff_loop(std::bind(&Storage::cacheCallback, this, std::placeholders::_1));
        } catch (exception e) {
            Log::warn("Сниффер просрал - создаем новый");
            sleep(1);
        }
    }
}

bool Storage::cacheCallback(const PDU &pdu) {
    RawPDU rawPDU = pdu.rfind_pdu<RawPDU>();

    CacheMsg_t *msg = (CacheMsg_t *)rawPDU.payload().data();
    if (rawPDU.size() != sizeof(CacheMsg_t)) {
        Log::warn("Mismatched size in cache msg");
        return true;
    }

    SessionIPv4 session;
    ipv4SessionsMutex.lock();
    if (ipv4Sessions.count(msg->dst_addr_v4)) {
        session = ipv4Sessions[msg->dst_addr_v4];
    }
    ipv4SessionsMutex.unlock();
    session.setSrc_addr_v4(msg->src_addr_v4);
    session.setDst_addr_v4(msg->dst_addr_v4);
    session.setIface(cfg->interfaceUnMorph(msg->ifName));
    ipv4SessionsMutex.lock();
    ipv4Sessions[msg->dst_addr_v4] = session;
    ipv4SessionsMutex.unlock();
    Log::debug("Caching session " + session.to_string());
    return true;
}

void Storage::cacheSession(SessionIPv4 session) {
    CacheMsg_t msg;
    strcpy(msg.ifName, cfg->interfaceMorph(session.ifName()).c_str());
    msg.src_addr_v4 = session.getSrc_addr_v4();
    msg.dst_addr_v4 = session.getDst_addr_v4();
    struct sockaddr_in cliaddr;
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(10000);
    cliaddr.sin_addr.s_addr = INADDR_BROADCAST;
    if (cache_sosk)
    sendto(cache_sosk, (const void*)&msg, sizeof(msg),
           0, (const struct sockaddr *) &cliaddr,
           sizeof(cliaddr));
}

void Storage::garpThreadRoutine() {
    while (1) {
        garpQueueMutex.lock();
        if (!garpQueue.size()) {
            garpQueueMutex.unlock();
            unique_lock<mutex> lk(garpLock);
            while (garpQueue.empty()) garpLockCv.wait(lk);
            continue;
        }
        auto gateway = garpQueue.front();
        garpQueue.pop_front();
        garpQueueMutex.unlock();
        auto ifaces = cfg->getInterfaces();
        for (auto iface: ifaces) {
            router->sendGarp(gateway.getAddress(), Helper::if_indexFromName(iface));
            this_thread::sleep_for(chrono::milliseconds(1));
        }
    }
}

bool Storage::isSessionMaster(in_addr_t addr) {
    if (ipv4Sessions.count(addr)) {
        return ipv4Sessions[addr].isMaster();
    }

    for (auto routesMap: ipv4StaticRoutes) {
        unordered_map<in_addr_t, StaticIPv4Route> routes = routesMap.second;
        for (auto route: routes) {
            if (route.second.isInNetwork(addr) &&
                route.second.isInstalled()) {
                return true;
            }
        }
    }

    return false;
}

void Storage::readStaticRoutes() {
    auto staticRoutes = cfg->getStaticRoutes();
    ipv4staticRoutesMutex.lock();
    auto old = ipv4StaticRoutes;
    ipv4StaticRoutes.clear();
    for (auto staticRoute: staticRoutes) {
        StaticIPv4Route route;
        route.setVia(staticRoute.second);
        route.setPrefix(staticRoute.first);
        ipv4StaticRoutes[route.getVia()][route.getNetwork()] = route;
    }
    /*for (auto oldRoutes: old) {
        for (auto oldRoute: oldRoutes.second) {
            bool needDel = true;
            StaticIPv4Route oldRealRoute = oldRoute.second;
            for (auto staticRoutes: ipv4StaticRoutes) {
                for (auto staticRoute: staticRoutes.second) {
                    StaticIPv4Route realRoute = staticRoute.second;
                    if (realRoute.getNetwork() == oldRealRoute.getNetwork() &&
                        realRoute.getMask() == oldRealRoute.getMask()) {
                        needDel = false;
                    }
                }
                if (!needDel) break;
            }
            if (needDel) {
                oldRealRoute.uninstall();
                pushStaticRouteToRouterCommander(oldRealRoute);
            }
        }
    }*/
    ipv4staticRoutesMutex.unlock();
}

unordered_map<in_addr_t, SessionIPv4> Storage::getIpv4Sessions() {
    ipv4SessionsMutex.lock();
    auto sessions = ipv4Sessions;
    ipv4SessionsMutex.unlock();
    return sessions;
}

void Storage::stop(string ip) {
    ipv4SessionsMutex.lock();
    in_addr_t addr = Helper::ipv4FromString(ip);
    if (!ipv4Sessions.count(addr)) {
        ipv4SessionsMutex.unlock();
        return;
    }
    auto session = ipv4Sessions[addr];
    session.setMaster(false);
    ipv4Sessions[addr] = session;
    pushSessionToRouterCommander(session);
    ipv4SessionsMutex.unlock();
    cacheSession(session);

}

vector<IPv4AddressInfo> Storage::getIpv4Gws() {
    ipv4GwsMutex.lock();
    auto gws = ipv4Gws;
    ipv4GwsMutex.unlock();
    return gws;
}

void Storage::savingThreadRoutine() {
    while (true) {
        this_thread::sleep_for(chrono::minutes(15));
        save();
    }
}








