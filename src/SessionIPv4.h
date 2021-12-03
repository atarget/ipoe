//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_SESSION_H
#define AUTHENTIFICATOR_SESSION_H

#include <vector>
#include <netinet/in.h>
#include <string>

using namespace std;

class SessionIPv4 {
public:
    in_addr_t getDst_addr_v4() const;

    void setDst_addr_v4(in_addr_t dst_addr_v4);

    in_addr_t getSrc_addr_v4() const;

    void setSrc_addr_v4(in_addr_t src_addr_v4);

    int getIf_index() const;

    void setIf_index(int if_index);

    bool isMaster() const;

    void setMaster(bool master);

    void setIface(const string &ifname);
    string ifName();
    string to_string();
    string dstAsString();
    string srsAsString();
private:
    string realHwAddrStr = "";
    string hwAddr = "";
public:
    const string &getRealHwAddr() const;
    void setRealHwAddr(const string &hwAddrStr);

    const string &getHwAddr() const;

    void setHwAddr(const string &hwAddr);

private:
    in_addr_t dst_addr_v4;
    in_addr_t src_addr_v4;
    int if_index;
    bool master = false;
};


#endif //AUTHENTIFICATOR_SESSION_H
