//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_ENGINE_H
#define AUTHENTIFICATOR_ENGINE_H


#include "Configuration.h"
#include "NetlinkListener.h"
#include "Storage.h"
#include "InterfaceListener.h"
#include "Console.h"

class Engine {
public:
    Engine(Configuration *cfg);
    void run();
private:
    Configuration *cfg;
    NetlinkListener *netlinkListener;
    Router *router;
    Storage *storage;
    Console *console;
    vector<InterfaceListener*> interfaceListeners;
    uint32_t control_if_index = 0;
    map<uint32_t, bool> if_indexes;

    void filterControlIface();
    void onAddressDelete(IPv4AddressInfo addressInfo);
    void onAddressAdd(IPv4AddressInfo addressInfo);
    void onRouteDelete(StaticIPv4Route route);
    void onRouteAdd(StaticIPv4Route route);

};


#endif //AUTHENTIFICATOR_ENGINE_H
