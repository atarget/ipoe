//
// Created by root on 03.09.19.
//

#ifndef AUTHENTIFICATOR_CONSOLE_H
#define AUTHENTIFICATOR_CONSOLE_H

#include <condition_variable>
#include "Configuration.h"
#include "Storage.h"
#include "Log.h"

typedef struct console_client {
    int sock;

} console_client_t ;

using namespace std;
class Console {
public:
    Console(Configuration *cfg, Storage *storage);
    void run();

private:
    Configuration *cfg;
    Storage *storage;
    int sock;


    int cli0 = 0;
    int cli1 = 0;
    int cli2 = 0;
    mutex cli0mtx;
    condition_variable cli0mtxCv;
    mutex cli1mtx;
    condition_variable cli1mtxCv;
    mutex cli2mtx;
    condition_variable cli2mtxCv;
    void clientsThread0Routine();
    void clientsThread1Routine();
    void clientsThread2Routine();
    bool login(int);
    void usage(int);
    void prompt(int);

    void cmd(string cmd, int cli);

    void cmd_sessions(int cli, bool master, bool slave);
    void cmd_stop(int cli, string ip);
    void cmd_gws(int cli);
    void cmd_save(int cli);
    void cmd_stat(int cli);
};


#endif //AUTHENTIFICATOR_CONSOLE_H
