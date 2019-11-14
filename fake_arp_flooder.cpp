

#include <tins/tins.h>
#include <netinet/in.h>
#include <iostream>
#include <thread>

using namespace Tins;
using namespace std;

int main(int argc, char **argv) {
    PacketSender sender;
    string iface = argv[1];
    string targetIp = argv[2];
    string startIp = argv[3];
    string cnt = argv[4];
    NetworkInterface oif(iface);
    in_addr_t startAddr = IPv4Address(startIp);
    in_addr_t addr = startAddr;
    int n = atoi(cnt.c_str());
    while (n--) {
        EthernetII arp = ARP::make_arp_request(targetIp, IPv4Address(addr), oif.hw_address());
        sender.send(arp, oif);
        cout << "SEND FROM " << IPv4Address(addr).to_string() << " TO " << targetIp << endl;
        addr = ntohl(htonl(addr)+1);
        this_thread::sleep_for(chrono::microseconds(12000));
    }
    cout << "DONE" << endl;
    return 0;
}