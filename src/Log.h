//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_LOG_H
#define AUTHENTIFICATOR_LOG_H

#include <string>
#include <mutex>
#include "Configuration.h"

using namespace std;
class Log {
public:
    static void Init(Configuration *cfg);
    static void debug(const string &message);
    static void info(const string &message);
    static void error(const string &message);
    static void warn(const string &message);
    static void emerg(const string &message);
private:
    static void check();
    static void log(int fac, const string &msg);


};




#endif //AUTHENTIFICATOR_LOG_H
