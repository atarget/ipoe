//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_STORAGE_H
#define AUTHENTIFICATOR_STORAGE_H

#include <map>
#include <vector>
#include <mutex>
#include <deque>
#include <unistd.h>
#include <thread>
#include <net/if.h>
#include <condition_variable>
#include "Configuration.h"
#include "SessionIPv4.h"
#include "StaticIPv4Route.h"
#include "IPv4AddressInfo.h"
#include "Router.h"

#define MAX_IF_NAME_SIZE 64
typedef struct CacheMsg {
    in_addr_t dst_addr_v4;
    in_addr_t src_addr_v4;
    char ifName[MAX_IF_NAME_SIZE];
} CacheMsg_t;

using json = nlohmann::json;
using namespace std;
class Storage {
public:
    Storage(Configuration *cfg, Router *router);
    /** save sessions to disk **/
    void save();
    /** load sessions from disk **/
    void load();
    /** put to router all sessions **/
    void requeueAllSessions();
    void requeueAllStaticRoutes();
    /** call from Interface listener **/
    void addIPv4Clent(IPv4AddressInfo address, string hwAddr, string realHwAddr);
    void routeDeleted(StaticIPv4Route route);
    void routeAdded(StaticIPv4Route route);
    void addControlGateway(IPv4AddressInfo gateway);
    void delControlGateway(IPv4AddressInfo gateway);
    bool isControlGateway(in_addr_t addr);
    IPv4AddressInfo getControlGwFor(in_addr_t addr);
    bool isSessionMaster(in_addr_t addr);
    void readStaticRoutes();
    bool cacheCallback(const PDU &pdu);
    unordered_map<in_addr_t, SessionIPv4> getIpv4Sessions();
    void stop(string ip);
    vector<IPv4AddressInfo> getIpv4Gws();

private:
    thread *cacheThread;
    /** udp cache server */
    void cacheRoutine();
    /** udp cache sender */
    void cacheSession(SessionIPv4 session);

    mutex ipv4SessionsMutex;
    /** client_addr, Session */
    unordered_map<in_addr_t, SessionIPv4> ipv4Sessions;

    mutex ipv4staticRoutesMutex;
    /** client_addr, Routes */
    unordered_map<in_addr_t, unordered_map<in_addr_t, StaticIPv4Route>> ipv4StaticRoutes;

    mutex ipv4GwsMutex;
    /** Info */
    vector<IPv4AddressInfo> ipv4Gws;


    mutex routerSessionsIPv4QueueLock;
    condition_variable routerSessionsIPv4QueueLockCv;
    mutex routerSessionsIPv4QueueMutex;
    deque<SessionIPv4> routerSessionsIPv4Queue;

    mutex routerStaticRoutesIPv4QueueLock;
    condition_variable routerStaticRoutesIPv4QueueLockCv;
    mutex routerStaticRoutesIPv4QueueMutex;
    deque<StaticIPv4Route> routerStaticRoutesIPv4Queue;

    Configuration *cfg;
    Router *router;

    void processStaticRoutesOf(in_addr_t addr, bool install);
    void pushStaticRouteToRouterCommander(StaticIPv4Route rt);
    void pushSessionToRouterCommander(SessionIPv4 sess);

    thread *routerCommanderStaticRoutesThread;
    thread *routerCommanderSessionsThread;
    void routerCommanderRoutine_StaticRoutesV4();
    void routerCommanderRoutine_SessionsV4();

    thread *garpThread;
    mutex garpLock;
    condition_variable garpLockCv;
    mutex garpQueueMutex;
    deque<IPv4AddressInfo> garpQueue;
    void garpThreadRoutine();


    int cache_srv_sock = 0;
    int cache_sosk = 0;

    thread *savingThread;
    void savingThreadRoutine();

    thread *controlGwStartThread;
    void controlGwStart();
};


#endif //AUTHENTIFICATOR_STORAGE_H
