//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_NETLINKLISTENER_H
#define AUTHENTIFICATOR_NETLINKLISTENER_H


#include <thread>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <deque>
#include <mutex>
#include <condition_variable>
#include "Log.h"

#include "Configuration.h"
#include "StaticIPv4Route.h"
#include "IPv4AddressInfo.h"

#define ROUTE_ADD_ACTION 1
#define ROUTE_DEL_ACTION 2
typedef struct routeAction {
    int action;
    StaticIPv4Route route;
} routeAction_t;

#define ADDRESS_ADD_ACTION 1
#define ADDRESS_DEL_ACTION 2
typedef struct addressAction {
    int action;
    IPv4AddressInfo address;
} addressAction_t;

#define BUFFER_SIZE 16000
class NetlinkListener {
public:
    NetlinkListener(Configuration *cfg);
    /**
     * ex onRouteAdd(std::bind(&aClass::test, a, _1));
     * aClass::test => void test(StaticIPv4Route action) {....
     * @param callback
     */
    void onRouteAdd(function<void(StaticIPv4Route action)> callback);
    /**
     * ex onRouteAdd(std::bind(&aClass::test, a, _1));
     * aClass::test => void test(StaticIPv4Route action) {....
     * @param callback
     */
    void onRouteDel(function<void(StaticIPv4Route action)> callback);
    /**
     * onAddressAdd(std::bind(&aClass::test, a, _1));
     * aClass::test => void test(IPv4AddressInfo action) {....
     * @param callback
     */
    void onAddressAdd(function<void(IPv4AddressInfo action)> callback);
    /**
     * onAddressAdd(std::bind(&aClass::test, a, _1));
     * aClass::test => void test(IPv4AddressInfo action) {....
     * @param callback
     */
    void onAddressDel(function<void(IPv4AddressInfo action)> callback);
private:
    mutex routeQueueLock;
    condition_variable routeQueueLockCv;
    mutex routeQueueMutex;
    deque<routeAction_t> routesQueue;

    mutex addressQueueLock;
    condition_variable addressQueueLockCv;
    mutex addressQueueMutex;
    deque<addressAction_t> addressQueue;

    thread *workingThread;
    Configuration *cfg;

    struct sockaddr_nl nl_addr;
    int nl_sock = 0;


    void initSocket();
    void routine();
    void processIPv4Route(struct nlmsghdr *nlh, int received_bytes);
    void processIPv4Address(struct nlmsghdr * nlh, int received_bytes);

    thread *addressQueueProcessing;
    thread *routeQueueProcessing;

    void addressQueueProcessingRoutine();
    void routeQueueProcessingRoutine();


    mutex routeAddListenersMutex;
    vector<function<void(StaticIPv4Route action)>> routeAddListeners;
    mutex routeDelListenersMutex;
    vector<function<void(StaticIPv4Route route)>> routeDelListeners;
    mutex addressAddListenersMutex;
    vector<function<void(IPv4AddressInfo action)>> addressAddListeners;
    mutex addressDelListenersMutex;
    vector<function<void(IPv4AddressInfo action)>> addressDelListeners;

};


#endif //AUTHENTIFICATOR_NETLINKLISTENER_H
