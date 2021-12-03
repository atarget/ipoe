//
// Created by root on 09.10.20.
//

#ifndef AUTHENTIFICATOR_DHCP_H
#define AUTHENTIFICATOR_DHCP_H
#include <stdlib.h>

typedef struct {
    u_int8_t op; // Message type defined in option field. 1 = REQUEST, 2 = REPLY
    u_int8_t htype; // Hardware address type and length of a DHCP client.
    u_int8_t hlen;
    u_int8_t hops; // Number of relay agents a request message traveled.
    u_int32_t xid; // Transaction ID, a random number chosen by the client to identify an IP address allocation
    u_int16_t secs; // Filled in by the client, the number of seconds elapsed since the client began address acquisition or renewal process. Currently this field is reserved and set to 0.
    u_int16_t flags; // The leftmost bit is defined as the BROADCAST (B) flag. If this flag is set to 0, the DHCP server sent a reply back by unicast; if this flag is set to 1, the DHCP server sent a reply back by broadcast. The remaining bits of the flags field are reserved for future use.
    u_int32_t ciaddr; // Client IP address.
    u_int32_t yiaddr; //'your' (client) IP address, assigned by the server.
    u_int32_t siaddr; //Server IP address, from which the client obtained configuration parameters.
    u_int32_t giaddr; //IP address of the first relay agent a request message traveled.
    u_int8_t chaddr[16]; // Client hardware address.
    u_int8_t sname[64]; //Server host name, from which the client obtained configuration parameters.
    u_int8_t file[128]; // Bootfile name and path information, defined by the server to the client.
    u_int8_t options[]; // Optional parameters field that is variable in length, which includes the message type, lease, domain name server IP address, and WINS IP address.
} DHCPHEADER;
#endif //AUTHENTIFICATOR_DHCP_H
