//
// Created by root on 03.09.19.
//

#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include "Router.h"
#include "Log.h"
#include "Helper.h"

void Router::processIPv4Session(SessionIPv4 session) {
    /**
     * if session master replace route else remove
     * 1. if master replace route
     * 2. if slave remove route
     * 3. if master - add garp request
     */
     if (session.isMaster()) {
         Log::debug("Session started " + session.to_string());
         replaceRoute(session.getDst_addr_v4(), 32, session.getSrc_addr_v4(), 0, session.getIf_index());
     } else {
         Log::debug("Session stopped " + session.to_string());
         removeRoute(session.getDst_addr_v4(), 32, session.getIf_index());
     }

}

void Router::processIPv4StaticRoute(StaticIPv4Route route) {
    /**
     * if route installed replace route else remove
     * 1. if master replace route
     * 2. if slave remove route
     * 3. if master - add garp request
     */
     if (route.isInstalled()) {
         replaceRoute(route.getNetwork(), route.getMask(), route.getSrc(), route.getVia(), route.getIf_index());
         Log::debug("Route installed " + route.to_string());
     } else {
         removeRoute(route.getNetwork(), route.getMask(), route.getIf_index());
         Log::debug("Route uninstalled " + route.to_string());
     }
}

int Router::addAttr(struct nlmsghdr *nl, int maxlen, int type, void *data, int attr_len) {
    struct rtattr *rta;
    int len = RTA_LENGTH(attr_len);
    if(NLMSG_ALIGN(nl->nlmsg_len) + len > maxlen)
    {
        perror("inside attr()");
        exit(EXIT_FAILURE);
    }
    rta = (struct rtattr *)((char *)nl + NLMSG_ALIGN(nl->nlmsg_len));
    //nl->nlmsg_len = NLMSG_ALIGN(nl->nlmsg_len) + len;
    rta->rta_type = type;
    rta->rta_len = len;
    memcpy(RTA_DATA(rta), data, attr_len);
    nl->nlmsg_len = NLMSG_ALIGN(nl->nlmsg_len) + len;
    return 0;
}

#define BUFSIZE 4095
void Router::replaceRoute(in_addr_t dst, uint8_t mask, in_addr_t src, in_addr_t via, uint32_t oif) {
    string log = "replace rt dst:" + to_string(dst) + " mask:" + to_string(mask) + " src:" + to_string(src) + " via:" + to_string(via) + " oif:" + to_string(oif);
    Log::debug(log);
    routerMutex.lock();
    if (via) {
        // @todo: сейчас такой костыль - надо понять как добавлять маршруты через нл с виа
        string cmd = "ip route replace " + IPv4Address(dst).to_string() + "/" + to_string(mask)
                + " via " + IPv4Address(via).to_string() + " src " + IPv4Address(src).to_string()
                + " dev " + Helper::if_indexToString(oif);
        system(cmd.c_str());
        routerMutex.unlock();
        return;
    }
    rt_msg_t msg;
    struct iovec iov;

    bzero(&msg, sizeof(msg));
    msg.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    msg.hdr.nlmsg_type = RTM_NEWROUTE;
    msg.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
    msg.hdr.nlmsg_seq = 1;
    msg.hdr.nlmsg_pid = 0;

    //fill entries for routing table

    msg.msg.rtm_family = AF_INET;
    msg.msg.rtm_dst_len = 32;
    msg.msg.rtm_src_len = 0;
    msg.msg.rtm_table = RT_TABLE_MAIN;
    msg.msg.rtm_protocol = RTPROT_STATIC;
    msg.msg.rtm_scope = RT_SCOPE_LINK;
    msg.msg.rtm_type = RTN_UNICAST;
    msg.msg.rtm_flags = 0;
    msg.msg.rtm_tos = 0;


    addAttr(&msg.hdr, BUFSIZE, RTA_DST, &dst, sizeof(in_addr_t));


    if (via) {
        addAttr(&msg.hdr, BUFSIZE, RTA_GATEWAY, &via, sizeof(in_addr_t));
    }

    if (src) {
        addAttr(&msg.hdr, BUFSIZE, RTA_PREFSRC, &src, sizeof(in_addr_t));
    }

    if (oif) {
        addAttr(&msg.hdr, BUFSIZE, RTA_OIF, &oif, sizeof(oif));
    }

    send(nl_sock, &msg, msg.hdr.nlmsg_len, 0);

    routerMutex.unlock();
}

Router::Router() {
    bzero(&nl_addr, sizeof(nl_addr));
    nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (nl_sock < 0) {
        Log::emerg("Error: can`t make netlink socket");
    }
    nl_addr.nl_family = AF_NETLINK;
    nl_addr.nl_groups = RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
    if (bind(nl_sock, (struct sockaddr*) &nl_addr, sizeof(nl_addr)) < 0) {
        Log::emerg("Error: can`t bind netlink socket");
    }
    arp_socket = socket( AF_INET, SOCK_PACKET, htons(ETH_P_RARP));
    if (arp_socket < 0) {
        Log::emerg("Error: can`t make arp socket");
    }
}

void Router::removeRoute(in_addr_t dst, uint8_t mask, uint32_t oif) {
    routerMutex.lock();
    if (mask != 32) {
        // @todo: сейчас такой костыль - надо понять как удалять маршруты через нл с маской не 32
        string cmd = "ip route delete " + IPv4Address(dst).to_string() + "/" + to_string(mask);
        system(cmd.c_str());
        routerMutex.unlock();
        return;
    }
    rt_msg_t msg;
    struct iovec iov;

    bzero(&msg, sizeof(msg));
    msg.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    msg.hdr.nlmsg_type = RTM_DELROUTE;
    msg.hdr.nlmsg_flags = NLM_F_REQUEST;
    msg.hdr.nlmsg_seq = 1;
    msg.hdr.nlmsg_pid = 0;

    //fill entries for routing table

    msg.msg.rtm_family = AF_INET;
    msg.msg.rtm_dst_len = 32;
    msg.msg.rtm_src_len = 0;
    msg.msg.rtm_table = RT_TABLE_MAIN;
    msg.msg.rtm_protocol = RTPROT_STATIC;
    msg.msg.rtm_scope = RT_SCOPE_LINK;
    msg.msg.rtm_type = RTN_UNICAST;
    msg.msg.rtm_flags = 0;
    msg.msg.rtm_tos = 0;

    addAttr(&msg.hdr, BUFSIZE, RTA_DST, &dst, sizeof(dst));
    if (oif) addAttr(&msg.hdr, BUFSIZE, RTA_OIF, &oif, sizeof(oif));
    send(nl_sock, &msg, msg.hdr.nlmsg_len, 0);
    routerMutex.unlock();
}

void Router::sendGarp(in_addr_t addr, uint32_t oif) {
    NetworkInterface iface = NetworkInterface::from_index(oif);
    IPv4Address address(addr);
    EthernetII garp = ARP::make_arp_request(address, address, iface.hw_address());
    struct sockaddr sa;
    strcpy(sa.sa_data, Helper::if_indexToString(oif).c_str());
    auto buf = garp.serialize();
    Log::debug("Sending GARP " + address.to_string() + " iface " + iface.name());
    sendto(arp_socket, buf.data(), buf.size(), 0, &sa, sizeof(sa));
}

int Router::getArp_socket() const {
    return arp_socket;
}
