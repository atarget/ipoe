//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_STATICROUTE_H
#define AUTHENTIFICATOR_STATICROUTE_H


#include <netinet/in.h>
#include <string>
#include <tins/tins.h>

using namespace std;
using namespace Tins;

class StaticIPv4Route {
public:
    void setPrefix(string prefix);
    void setNetwork(string network);
    void setNetwork(uint32_t network);
    void setVia(string via);
    void setVia(uint32_t via);
    void setMask(uint8_t mask);
    void setSrc(string src);
    void setSrc(uint32_t src);
    void setInterface(string name);
    void setIfIndex(uint32_t if_index);
    bool isInNetwork(in_addr_t addr);

    in_addr_t getNetwork() const;
    uint8_t getMask() const;
    in_addr_t getBcast() const;
    in_addr_t getVia() const;
    in_addr_t getSrc() const;
    uint32_t getIf_index() const;

    string stringNetwork() const;
    string stringSrc() const;
    string stringVia() const;
    string stringBcast() const;
    string stringPrefix() const;

    string interfaceName() const;
    /**
     * modify property only
     */
    void install();
    void uninstall();
    bool isInstalled();

    const string to_string();
private:
    in_addr_t network;
    uint8_t mask;
    uint32_t bitmask;
    in_addr_t bcast;
    in_addr_t via;
    in_addr_t src;
    uint32_t if_index;
    bool installed;
};


#endif //AUTHENTIFICATOR_STATICROUTE_H
