//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_ROUTER_H
#define AUTHENTIFICATOR_ROUTER_H

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <mutex>
#include <iostream>
#include "SessionIPv4.h"
#include "StaticIPv4Route.h"

using namespace std;
typedef struct rtMsg {
    struct nlmsghdr hdr;
    struct rtmsg msg;
    char buffer[4096];
} rt_msg_t;

class Router {
public:
    Router();

    int getArp_socket() const;

    void processIPv4Session(SessionIPv4 session);
    void processIPv4StaticRoute(StaticIPv4Route route);
    void sendGarp(in_addr_t addr, uint32_t oif);
private:
    int arp_socket;
    int addAttr(struct nlmsghdr *nl, int maxlen, int type, void *data, int attr_len);
    void replaceRoute(in_addr_t dst, uint8_t mask, in_addr_t src, in_addr_t via, uint32_t oif);
    void removeRoute(in_addr_t dst, uint8_t mask, uint32_t oif);
    int nl_sock = 0;
    struct sockaddr_nl nl_addr;
    mutex routerMutex;

};


#endif //AUTHENTIFICATOR_ROUTER_H
