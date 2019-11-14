//
// Created by root on 03.09.19.
//

#include <iostream>
#include <vector>
#include "Configuration.h"
#include <fstream>
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
    interfaces = (*j)["interfaces"].get<vector<string>>();
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

void Configuration::config_skipProxyArp(const json *j) {
    if (!j->count("skipProxyArp")) return;
    vector<string> ignore = (*j)["skipProxyArp"].get<vector<string>>();

    for (auto ip: ignore) {
        skipProxyArp[Helper::ipv4FromString(ip)] = Helper::ipv4FromString(ip);
    }
}

map<uint32_t, uint32_t> &Configuration::getSkipProxyArp() {
    return skipProxyArp;
}




