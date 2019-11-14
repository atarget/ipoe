//
// Created by root on 03.09.19.
//

#include <iostream>
#include <syslog.h>
#include "Log.h"

Configuration *Log_cfg;
void Log::Init(Configuration *cfg) {
    Log_cfg = cfg;
    if (!cfg->isForeground()) {
        openlog("ipoe", LOG_PID, LOG_DAEMON);
    }
}

void Log::info(const string &message) {
    if (Log_cfg && Log_cfg->getLoglevel() < 4) return;
    log(LOG_INFO, "INFO: " + message);
}

void Log::check() {
    if (!Log_cfg) {
        cerr << "Log system does not initialized - exit" << endl;
        exit(EXIT_FAILURE);
    }
}

mutex logging;
void Log::log(int fac, const string &msg) {
    logging.lock();
    check();
    if (!Log_cfg->isForeground())
        syslog(fac, "%s", msg.c_str());
    else
        cout << msg << endl;
    logging.unlock();
}

void Log::error(const string &message) {
    if (Log_cfg && Log_cfg->getLoglevel() < 2) return;
    log(LOG_ERR, "ERR: " + message);
}

void Log::warn(const string &message) {
    if (Log_cfg && Log_cfg->getLoglevel() < 3) return;
    log(LOG_WARNING, "WARN: " +message);
}

void Log::emerg(const string &message) {
    if (Log_cfg && Log_cfg->getLoglevel() < 1) return;
    if (!Log_cfg)
        cout << "EMRG: " << message << endl;
    else
        log(LOG_EMERG, "EMRG: " + message);
    exit(EXIT_FAILURE);
}

void Log::debug(const string &message) {
    if (Log_cfg && Log_cfg->getLoglevel() < 5) return;
    log(LOG_DEBUG, "DBG: " +message);
}
