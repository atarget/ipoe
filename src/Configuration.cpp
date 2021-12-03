//
// Created by root on 03.09.19.
//

#include <iostream>
#include <vector>
#include "Configuration.h"
#include <fstream>
#include <regex>
#include "Log.h"
#include "Helper.h"


Configuration::Configuration(const string &filename) {
    json j;
    ifstream f(filename);
    f >> j;
    f.close();
    config_log(&j);
    config_ControlInterface(&j);
    config_Interfaces(&j);
    config_Cli(&j);
    config_Cache(&j);
    config_StaticRoutes(&j);
    config_BPF(&j);
    config_InterfaceBinding(&j);
    config_skipProxyArp(&j);
    config_realy_iface(&j);
}

char *Configuration::findArgument(char **startArgv, char **endArgv, const string &need) {
    char **itr = std::find(startArgv, endArgv, need);
    if (*itr != NULL && *(++itr) != NULL) {
        return *itr;
    }
    return NULL;
}

void Configuration::usage(const char *argv0) {
    cout << "Usage: " << argv0 << " -c <cfg file name> [-f] [-p <pid file>]" << endl;
    cout << "\t-c <cfg file name> - use configuration from file" << endl;
    cout << "\t-p <pid file> - use configuration from file" << endl;
    cout << "\t-f - run foreground" << endl;
    exit(-1);
}

bool Configuration::isArgumentExists(char **startArgv, char **endArgv, const string &need) {
    return *std::find(startArgv, endArgv, need) != NULL;
}

Configuration Configuration::make(int argc, char **argv) {
    char *cfgFileName = Configuration::findArgument(argv, argv+argc, "-c");
    if (!cfgFileName || Configuration::isStrArgument(cfgFileName)) {
        Configuration::usage(argv[0]);
    }
    Configuration configuration(cfgFileName);
    if (Configuration::isArgumentExists(argv, argv+argc, "-p")) {
        char *pidFileName = Configuration::findArgument(argv, argv + argc, "-p");
        if (!pidFileName || Configuration::isStrArgument(cfgFileName)) Configuration::usage(argv[0]);
        configuration.setPidFileName(pidFileName);
    }
    if (Configuration::isArgumentExists(argv, argv+argc, "-f")) {
        configuration.setForeground();
    }
    return configuration;
}

void Configuration::setPidFileName(const char *fileName) {
    m_pidFileName = fileName;
}

void Configuration::setForeground() {
    foreground = true;
}

bool Configuration::isStrArgument(const string &testStr) {
    vector<string> args;
    args.push_back("-c");
    args.push_back("-f");
    args.push_back("-p");
    auto itr = std::find(args.begin(), args.end(), testStr);
    if (itr != args.end())
        return true;
    return false;
}

bool Configuration::isForeground() {
    return foreground;
}

const char *Configuration::getPidFileName() {
    return m_pidFileName;
}

void Configuration::config_ControlInterface(const json *j) {
    if (j->count("controlInterface") == 0) {
        Log::emerg("config error: need controlInterface");
    }
    controlInterface = (*j)["controlInterface"].get<std::string>();
}

void Configuration::config_Interfaces(const json *j) {
    if (j->count("interfaces") == 0) {
        Log::emerg("config error: need interfaces");
    }
    vector<string> m_interfaces = (*j)["interfaces"].get<vector<string>>();
    for (auto iface: m_interfaces) {
        regex  expr("([a-z0-9A-Z]+)\\.([0-9\\-]+)");
        smatch m;
        if (regex_search(iface, m, expr)) {
                if (m.size() == 3) {
                    string tmp = m[2];
                    int p = 0;
                    if ((p = tmp.find("-")) != string::npos) {
                        string minVlan = tmp.substr(0, p);
                        string maxVlan = tmp.substr(p+1);
                        for (int i = std::stoi(minVlan); i <= std::stoi(maxVlan); i++) {
                            const uint32_t if_index = Helper::if_indexFromName((string)m[1] + "." + std::to_string(i));
                            if (if_index) {
                                interfaces.push_back((string)m[1] + "." + std::to_string(i));
                            }
                        }
                    } else {
                        interfaces.push_back(iface);
                    }
                }
        }
    }
}

void Configuration::dumpConfig() {
    Log::info("Control interface: " + controlInterface);
    for (auto iface : interfaces) {
        Log::info("Add client interface: " + iface);
    }
    Log::info("Cli on " + cliAddress + ":" + to_string(cliPort));
    Log::info("Cache on port " + to_string(cachePort));
}

void Configuration::config_Cli(const json *j) {
    if (!j->count("cli")) return;
    auto cli = (*j)["cli"];
    if (cli.count("port")) {
        cliPort = cli["port"].get<int>();
    }
}

void Configuration::config_Cache(const json *j) {
    if (!j->count("cache")) return;
    auto cache = (*j)["cache"];
    if (cache.count("port")) {
        cachePort = cache["port"].get<int>();
    }
    if (cache.count("fileName")) {
        cacheFileName = cache["fileName"].get<string>();
    }
    if (cache.count("bind")) {
        cacheBind = cache["bind"].get<string>();
    } else {
        Log::emerg("bind option for cache does not setted");
    }
}

void Configuration::config_StaticRoutes(const json *j) {
    if (!j->count("staticRoutes")) return;
    json::object_t jStaticRoutes = (*j)["staticRoutes"].get<json::object_t>();
    for (auto staticRoute: jStaticRoutes) {
        string prefix = staticRoute.first;
        string via = staticRoute.second.get<string>();
        staticRoutes[prefix] = via;
    }

}

void Configuration::config_realy_iface(const json *j) {
    if (!j->count("relay_iface")) return;
    realy_iface = (*j)["relay_iface"].get<string>();
}

void Configuration::config_BPF(const json *j) {
    if (!j->count("bpf")) return;
    bpf = (*j)["bpf"].get<string>();
}


const string &Configuration::getControlInterface() const {
    return controlInterface;
}

const vector<string> &Configuration::getInterfaces() const {
    return interfaces;
}

const string &Configuration::getCacheFileName() const {
    return cacheFileName;
}

const string &Configuration::getBpf() const {
    return bpf;
}

int Configuration::getCliPort() const {
    return cliPort;
}

const string &Configuration::getCliAddress() const {
    return cliAddress;
}

int Configuration::getCachePort() const {
    return cachePort;
}

const string &Configuration::getCacheBind() const {
    return cacheBind;
}

void Configuration::config_InterfaceBinding(const json *j) {
    if (!j->count("interfaceBinding")) return;
    json::object_t interfaceBinding = (*j)["interfaceBinding"].get<json::object_t>();
    for (auto binding : interfaceBinding) {
        string iface = binding.first;
        string name = binding.second.get<string>();
        morphers[iface] = name;
        unmorphers[name] = iface;
    }

}

string Configuration::interfaceMorph(string name) {
    if (morphers.count(name)) return morphers[name];
    return name;
}

string Configuration::interfaceUnMorph(string name) {
    if (unmorphers.count(name)) return unmorphers[name];
    return name;
}

const map<string, string> &Configuration::getStaticRoutes() const {
    return staticRoutes;
}

void Configuration::config_log(const json *j) {
    if (!j->count("loglevel")) return;
    loglevel = (*j)["loglevel"].get<int>();
}

int Configuration::getLoglevel() const {
    return loglevel;
}

void Configuration::processIpPart(string already, string part, vector<string> *vector) {

}

void Configuration::config_skipProxyArp(const json *j) {
    if (!j->count("skipProxyArp")) return;
    vector<string> ignore_raw = (*j)["skipProxyArp"].get<vector<string>>();
    for (auto ip: ignore_raw) {
        regex  expr("([0-9\\-]+)\\.([0-9\\-]+)\\.([0-9\\-]+)\\.([0-9\\-]+)");
        smatch m;
        if (regex_search(ip, m, expr)) {
            if (m.size() == 5) {
                string b0 = m[1];
                int b0Min, b0Max, b0Div;
                if ((b0Div = b0.find("-")) == string::npos) {
                    b0Min = std::stoi(b0);
                    b0Max = std::stoi(b0);
                } else {
                    b0Min = std::stoi(b0.substr(0, b0Div));
                    b0Max = std::stoi(b0.substr(b0Div + 1));
                }

                string b1 = m[2];
                int b1Min, b1Max, b1Div;
                if ((b1Div = b1.find("-")) == string::npos) {
                    b1Min = std::stoi(b1);
                    b1Max = std::stoi(b1);
                } else {
                    b1Min = std::stoi(b1.substr(0, b1Div));
                    b1Max = std::stoi(b1.substr(b1Div + 1));
                }

                string b2 = m[3];
                int b2Min, b2Max, b2Div;
                if ((b2Div = b2.find("-")) == string::npos) {
                    b2Min = std::stoi(b2);
                    b2Max = std::stoi(b2);
                } else {
                    b2Min = std::stoi(b2.substr(0, b2Div));
                    b2Max = std::stoi(b2.substr(b2Div + 1));
                }

                string b3 = m[4];
                int b3Min, b3Max, b3Div;
                if ((b3Div = b3.find("-")) == string::npos) {
                    b3Min = std::stoi(b3);
                    b3Max = std::stoi(b3);
                } else {
                    b3Min = std::stoi(b3.substr(0, b3Div));
                    b3Max = std::stoi(b3.substr(b3Div + 1));
                }

                for (int b0i = b0Min; b0i <= b0Max; b0i++) {
                    for (int b1i = b1Min; b1i <= b1Max; b1i++) {
                        for (int b2i = b2Min; b2i <= b2Max; b2i++) {
                            for (int b3i = b3Min; b3i <= b3Max; b3i++) {
                                skipProxyArp[Helper::ipv4FromString(
                                        std::to_string(b0i) + "." +
                                             std::to_string(b1i) + "." +
                                             std::to_string(b2i) + "." +
                                             std::to_string(b3i)
                                             )] = Helper::ipv4FromString(std::to_string(b0i) + "." +
                                                     std::to_string(b1i) + "." +
                                                     std::to_string(b2i) + "." +
                                                     std::to_string(b3i));
                            }
                        }
                    }
                }

            }
        }
    }

    vector<string> ignore = (*j)["skipProxyArp"].get<vector<string>>();

    for (auto ip: ignore) {
        skipProxyArp[Helper::ipv4FromString(ip)] = Helper::ipv4FromString(ip);
    }
}

map<uint32_t, uint32_t> &Configuration::getSkipProxyArp() {
    return skipProxyArp;
}

const string &Configuration::getRealyIface() const {
    return realy_iface;
}




