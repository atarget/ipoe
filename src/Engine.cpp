//
// Created by root on 03.09.19.
//

#include "Engine.h"

Engine::Engine(Configuration *cfg) {
    this->cfg = cfg;
}

void Engine::run() {
    string flushArpTables = "arptables -F OUTPUT";
    system(flushArpTables.c_str());
    control_if_index = Helper::if_indexFromName(cfg->getControlInterface());
    for (auto IfName: cfg->getInterfaces()) {
        if_indexes[Helper::if_indexFromName(IfName)] = true;
    }
    router = new Router();
    storage = new Storage(cfg, router);
    netlinkListener = new NetlinkListener(cfg);
    netlinkListener->onAddressAdd(std::bind(&Engine::onAddressAdd, this, std::placeholders::_1));
    netlinkListener->onAddressDel(std::bind(&Engine::onAddressDelete, this, std::placeholders::_1));
    netlinkListener->onRouteAdd(std::bind(&Engine::onRouteAdd, this, std::placeholders::_1));
    netlinkListener->onRouteDel(std::bind(&Engine::onRouteDelete, this, std::placeholders::_1));
    Log::info("Loading cache");
    storage->load();
    Log::info("Installing cache");
    storage->requeueAllSessions();
    Log::info("Installing static");
    storage->requeueAllStaticRoutes();
    for (auto iface : cfg->getInterfaces()) {
        const uint32_t if_index = Helper::if_indexFromName(iface);
        if (if_index) {
            interfaceListeners.push_back(new InterfaceListener(if_index, storage, cfg, router->getArp_socket()));
        } else {
            Log::warn("Iface " + iface + " is invalid");
        }
    }
    filterControlIface();
    Log::info("Started");
    console = new Console(cfg, storage);
    console->run();

    while (1) sleep(100000);
}

void Engine::onAddressDelete(IPv4AddressInfo addressInfo) {
    if (control_if_index == addressInfo.getIf_index())
        storage->delControlGateway(addressInfo);
}

void Engine::onAddressAdd(IPv4AddressInfo addressInfo) {
    if (control_if_index == addressInfo.getIf_index())
        storage->addControlGateway(addressInfo);
}

void Engine::onRouteDelete(StaticIPv4Route route) {
    if (if_indexes.count(route.getIf_index()))
        storage->routeDeleted(route);
}

void Engine::onRouteAdd(StaticIPv4Route route) {
    if (if_indexes.count(route.getIf_index()))
        storage->routeAdded(route);
}

void Engine::filterControlIface() {
    string cmd = "arptables -D OUTPUT -j DROP -o " + cfg->getControlInterface();
    system(cmd.c_str());
    cmd = "arptables -I OUTPUT -j DROP -o " + cfg->getControlInterface();
    system(cmd.c_str());
}
