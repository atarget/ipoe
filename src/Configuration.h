//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_CONFIGURATION_H
#define AUTHENTIFICATOR_CONFIGURATION_H

#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <netinet/in.h>

using namespace std;
using json = nlohmann::json;
class Configuration {
public:
    bool isForeground();
    const char *getPidFileName();
    void dumpConfig();
    const string &getControlInterface() const;
    const vector<string> &getInterfaces() const;
    const string &getBpf() const;
    const string &getRealyIface() const;
    const string &getCacheFileName() const;
    int getCliPort() const;
    const string &getCliAddress() const;
    int getCachePort() const;
    const string &getCacheBind() const;

    int getLoglevel() const;

    const map<string, string> &getStaticRoutes() const;

    string interfaceMorph(string name);
    string interfaceUnMorph(string name);

    static void usage(const char *);
    static char* findArgument(char **startArgv, char **endArgv, const string &need);
    static bool isArgumentExists(char **startArgv, char **endArgv, const string &need);
    static Configuration make(int argc, char **argv);
    static bool isStrArgument(const string &testStr);

    map<in_addr_t, in_addr_t> &getSkipProxyArp();


private:
    const char *m_pidFileName;
    bool foreground = false;

    Configuration(const string &filename);
    void setPidFileName(const char *fileName);
    void setForeground();
    map<uint32_t, uint32_t> skipProxyArp;
    string controlInterface;
    vector<string> interfaces;
    string bpf = "(udp and src port 500 and dst port 500) or (udp and dst port 4500 and src port 4500) or (arp and arp[6:2] = 1) or (tcp[tcpflags] & (tcp-syn) != 0)";

    string cliAddress = "127.0.0.1";
    int cliPort = 9999;

    int cachePort = 10000;
    string cacheFileName = "ipoe.cache";
    string cacheBind = "";
    int loglevel = 5;
    string realy_iface = "bond.100";

    map<string, string> staticRoutes;

    void config_ControlInterface(const json *j);
    void config_Interfaces(const json *j);
    void config_InterfaceBinding(const json *j);
    void config_Cli(const json *j);
    void config_Cache(const json *j);
    void config_StaticRoutes(const json *j);
    void config_BPF(const json *j);
    void config_log(const json *j);
    void config_skipProxyArp(const json *j);
    void config_realy_iface(const json *j);
    void processIpPart(string already, string part, vector<string> *vector);

    unordered_map<string, string> morphers;
    unordered_map<string, string> unmorphers;

};



#endif //AUTHENTIFICATOR_CONFIGURATION_H
