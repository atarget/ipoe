#include <iostream>
#include "src/Configuration.h"
#include "src/Log.h"
#include "src/NetlinkListener.h"
#include "src/Engine.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fstream>
#include <sys/socket.h>

int main(int argc, char **argv) {
    Configuration configuration = Configuration::make(argc, argv);
    const char* cwd = get_current_dir_name();
    if (!configuration.isForeground()) {
        pid_t pid = fork();
        // error
        if (pid < 0) exit(EXIT_FAILURE);
        // parent finished with success
        if (pid > 0) exit(EXIT_SUCCESS);
        // child becomes an session leader
        if (setsid() < 0) exit(EXIT_FAILURE);
        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);
        // second time fork
        pid = fork();
        // error
        if (pid < 0) exit(EXIT_FAILURE);
        // exit parent
        if (pid > 0) {

            exit(EXIT_SUCCESS);
        }
        // set new file permissions
        umask(0);
        // change dir - now root will be other
        chdir(cwd);
        // close all files
        for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
            close(x);
        }
    }
    const char *pidFileName = configuration.getPidFileName();
    if (pidFileName) {
        ofstream pidFile(pidFileName);
        pidFile << getpid() << endl;
        pidFile.close();
    }
    Log::Init(&configuration);
    Log::info("IPOE daemon started");
    configuration.dumpConfig();
    Engine engine(&configuration);
    engine.run();
    return 0;
}