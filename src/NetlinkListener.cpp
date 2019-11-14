//
// Created by root on 03.09.19.
//


#include "NetlinkListener.h"
#include "StaticIPv4Route.h"
#include "IPv4AddressInfo.h"


NetlinkListener::NetlinkListener(Configuration *cfg) {
    this->cfg = cfg;
    initSocket();
    workingThread = new thread(&NetlinkListener::routine, this);
    addressQueueProcessing = new thread(&NetlinkListener::addressQueueProcessingRoutine, this);
    routeQueueProcessing = new thread(&NetlinkListener::routeQueueProcessingRoutine, this);
}

void NetlinkListener::initSocket() {
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
}


void NetlinkListener::routine() {
    while (true) {
        int received_bytes = 0;
        struct nlmsghdr *nlh;
        char buffer[BUFFER_SIZE];
        bzero(buffer, sizeof(buffer));
        char tmp[1024];
        bzero(tmp, sizeof(tmp));
        /* Receiving netlink socket data */
        while (1) {
            received_bytes = recv(nl_sock, buffer, sizeof(buffer), 0);
            if (received_bytes < 0) continue;
            /* cast the received buffer */
            nlh = (struct nlmsghdr *) buffer;
            /* If we received all data ---> break */
            if (nlh->nlmsg_type == NLMSG_DONE)
                break;
            /* We are just intrested in Routing information */
            if (nl_addr.nl_groups & (RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR))
                break;
        }

        /* Reading netlink socket data */
        /* Loop through all entries */
        /* For more informations on some functions :
         * http://www.kernel.org/doc/man-pages/online/pages/man3/netlink.3.html
         * http://www.kernel.org/doc/man-pages/online/pages/man7/rtnetlink.7.html
         */
        if (nl_addr.nl_groups & RTMGRP_IPV4_ROUTE) {
            processIPv4Route(nlh, received_bytes);
        }

        if (nl_addr.nl_groups & RTMGRP_IPV4_IFADDR) {
            processIPv4Address(nlh, received_bytes);
        }
    }
}

void NetlinkListener::processIPv4Route(struct nlmsghdr *nlh, int received_bytes) {
    struct rtmsg *route_entry;
    struct rtattr *route_attribute;
    int route_attribute_len = 0;
    char tmp[1024];
    bzero(tmp, sizeof(tmp));
    if (nlh->nlmsg_type != RTM_DELROUTE && nlh->nlmsg_type != RTM_NEWROUTE) {
        // not this handler need
        return;
    }
    StaticIPv4Route route;
    for (; NLMSG_OK(nlh, received_bytes);
           nlh = NLMSG_NEXT(nlh, received_bytes)) {
        /* Get the route data */
        route_entry = (struct rtmsg *) NLMSG_DATA(nlh);
        route.setMask(route_entry->rtm_dst_len);
        /* Get attributes of route_entry */
        route_attribute = RTM_RTA(route_entry);
        /* Get the route atttibutes len */
        route_attribute_len = RTM_PAYLOAD(nlh);
        /* Loop through all attributes */
        for (; RTA_OK(route_attribute, route_attribute_len);
               route_attribute = RTA_NEXT(route_attribute, route_attribute_len)) {
            if (route_attribute->rta_type == RTA_DST) {
                route.setNetwork(*(uint32_t*)RTA_DATA(route_attribute));
            }
            if (route_attribute->rta_type == RTA_GATEWAY) {
                route.setVia(*(uint32_t*)RTA_DATA(route_attribute));
            }
            if (route_attribute->rta_type == RTA_PREFSRC) {
                route.setSrc(*(uint32_t*)RTA_DATA(route_attribute));
            }
            if (route_attribute->rta_type == RTA_OIF) {
                route.setIfIndex(*(uint32_t *) RTA_DATA(route_attribute));
            }
        }
        routeAction_t action;

        action.route = route;
        if (nlh->nlmsg_type == RTM_DELROUTE) {
            action.action = ROUTE_DEL_ACTION;
            routeQueueMutex.lock();
            routesQueue.push_back(action);
            routeQueueMutex.unlock();
            routeQueueLockCv.notify_one();
        }
        if (nlh->nlmsg_type == RTM_NEWROUTE) {
            action.action = ROUTE_ADD_ACTION;
            routeQueueMutex.lock();
            routesQueue.push_back(action);
            routeQueueMutex.unlock();
            routeQueueLockCv.notify_one();
        }

    }
}

void NetlinkListener::processIPv4Address(struct nlmsghdr *nlh, int received_bytes) {
    struct ifaddrmsg *ifaddr_entry;
    struct rtattr *addr_attribute;
    uint32_t addr_attributes_len = 0;
    char tmp[BUFFER_SIZE];
    IPv4AddressInfo addressInfo;

    if (nlh->nlmsg_type != RTM_DELADDR && nlh->nlmsg_type != RTM_NEWADDR) {
        return;
    }

    for (; NLMSG_OK(nlh, received_bytes);
           nlh = NLMSG_NEXT(nlh, received_bytes)) {
        /* Get the route data */
        ifaddr_entry = (struct ifaddrmsg *) NLMSG_DATA(nlh);
        addressInfo.setIf_index(ifaddr_entry->ifa_index);
        addressInfo.setMask(ifaddr_entry->ifa_prefixlen);

        addr_attribute = IFA_RTA(ifaddr_entry);
        addr_attributes_len = RTM_PAYLOAD(nlh);

        /* Loop through all attributes */
        for (; RTA_OK(addr_attribute, addr_attributes_len);
               addr_attribute = RTA_NEXT(addr_attribute, addr_attributes_len)) {
            if (addr_attribute->rta_type == IFLA_ADDRESS) {
                addressInfo.setAddress(*(uint32_t*)RTA_DATA(addr_attribute));
            }
        }
        addressAction_t action;
        action.address = addressInfo;
        if (nlh->nlmsg_type == RTM_DELADDR) {
            action.action = ADDRESS_DEL_ACTION;
            addressQueueMutex.lock();
            addressQueue.push_back(action);
            addressQueueMutex.unlock();
            addressQueueLockCv.notify_one();
        }


        if (nlh->nlmsg_type == RTM_NEWADDR) {
            action.action = ADDRESS_ADD_ACTION;
            addressQueueMutex.lock();
            addressQueue.push_back(action);
            addressQueueMutex.unlock();
            addressQueueLockCv.notify_one();
        }
    }

}

void NetlinkListener::addressQueueProcessingRoutine() {
    while (true) {
        addressQueueMutex.lock();
        int qSize = addressQueue.size();
        addressQueueMutex.unlock();
        while (qSize > 0) {
            addressQueueMutex.lock();
            addressAction_t action = addressQueue.front();
            addressQueue.pop_front();
            qSize = addressQueue.size();
            addressQueueMutex.unlock();
            if (action.action == ADDRESS_ADD_ACTION) {
                addressAddListenersMutex.lock();
                for (auto callback: addressAddListeners) {
                    callback(action.address);
                }
                addressAddListenersMutex.unlock();
            }
            if (action.action == ADDRESS_DEL_ACTION) {
                addressDelListenersMutex.lock();
                for (auto callback: addressDelListeners) {
                    callback(action.address);
                }
                addressDelListenersMutex.unlock();
            }
        }
        unique_lock<mutex> lk(addressQueueLock);
        while (addressQueue.empty()) addressQueueLockCv.wait(lk);
    }
}

void NetlinkListener::routeQueueProcessingRoutine() {
    while (true) {
        routeQueueMutex.lock();
        int qSize = routesQueue.size();
        routeQueueMutex.unlock();
        while (qSize > 0) {
            routeQueueMutex.lock();
            routeAction_t action = routesQueue.front();
            routesQueue.pop_front();
            qSize = routesQueue.size();
            routeQueueMutex.unlock();
            if (action.action == ROUTE_ADD_ACTION) {
                routeAddListenersMutex.lock();
                for (auto callback: routeAddListeners) {
                    callback(action.route);
                }
                routeAddListenersMutex.unlock();
            }
            if (action.action == ROUTE_DEL_ACTION) {
                routeDelListenersMutex.lock();
                for (auto callback: routeDelListeners) {
                    callback(action.route);
                }
                routeDelListenersMutex.unlock();
            }
        }
        unique_lock<mutex> lk(routeQueueLock);
        while (routesQueue.empty()) routeQueueLockCv.wait(lk);
    }
}

void NetlinkListener::onRouteAdd(function<void(StaticIPv4Route action)> callback) {
    routeAddListenersMutex.lock();
    routeAddListeners.push_back(callback);
    routeAddListenersMutex.unlock();
}

void NetlinkListener::onRouteDel(function<void(StaticIPv4Route action)> callback) {
    routeDelListenersMutex.lock();
    routeDelListeners.push_back(callback);
    routeDelListenersMutex.unlock();
}

void NetlinkListener::onAddressAdd(function<void(IPv4AddressInfo action)> callback) {
    addressAddListenersMutex.lock();
    addressAddListeners.push_back(callback);
    addressAddListenersMutex.unlock();
}

void NetlinkListener::onAddressDel(function<void(IPv4AddressInfo action)> callback) {
    addressDelListenersMutex.lock();
    addressDelListeners.push_back(callback);
    addressDelListenersMutex.unlock();
}



