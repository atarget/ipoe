//
// Created by root on 03.09.19.
//

#include "Console.h"

Console::Console(Configuration *cfg, Storage *storage) : cfg(cfg), storage(storage) {

}

void Console::run() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    int trueflag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&trueflag, sizeof(trueflag));
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = IPv4Address(cfg->getCliAddress());
    addr.sin_port = htons( cfg->getCliPort() );
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 3);
    int addrlen = sizeof(addr);
    new thread(&Console::clientsThread0Routine, this);
    new thread(&Console::clientsThread1Routine, this);
    new thread(&Console::clientsThread2Routine, this);

    while (true) {
        int cli = accept(sock, (struct sockaddr *) &addr, (socklen_t*)&addrlen);
        int timeout = 1;
        setsockopt(cli, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        if (cli0 && cli1 && cli2) {
            char error[] = "Connection limit reached\n";
            write(cli, &error, strlen(error) + 1);
            close(cli);
            continue;
        }
        if (!cli0) {
            cli0 = cli;
            cli0mtxCv.notify_one();
            continue;
        }
        if (!cli1) {
            cli1 = cli;
            cli1mtxCv.notify_one();
            continue;
        }
        if (!cli2) {
            cli2 = cli;
            cli2mtxCv.notify_one();
            continue;
        }
    };
}

bool Console::login(int sock) {
    cmd_stat(sock);
    return true;
}

void Console::clientsThread0Routine() {
    while (true) {
        unique_lock<mutex> lk(cli0mtx);
        while (!cli0) cli0mtxCv.wait(lk);
        try {
        if (!login(cli0)) {
            close(cli0);
            cli0 = 0;
            continue;
        }
        while (true) {
            char command[4096];
            bzero(command, 4096);
            prompt(cli0);
            int n = read(cli0, &command, 4095);
            if (n < 0 || n == 0) {
                close(cli0);
                cli0 = 0;
                break;
            }
            command[strlen(command) - 2] = 0;
            cmd(command, cli0);
        }
        } catch (exception e) {}
    }
}

void Console::clientsThread1Routine() {

    while (true) {
        unique_lock<mutex> lk(cli1mtx);
        while (!cli1) cli1mtxCv.wait(lk);
        try {
        if (!login(cli1)) {
            close(cli1);
            cli1 = 0;
            continue;
        }
        while (true) {
            char command[4096];
            bzero(command, 4096);
            prompt(cli1);
            int n = read(cli1, &command, 4095);
            if (n < 0 || n == 0) {
                close(cli1);
                cli1 = 0;
                break;
            }
            command[strlen(command) - 2] = 0;
            cmd(command, cli1);
        }
        } catch (exception e) {}
    }
}

void Console::clientsThread2Routine() {

    while (true) {
        unique_lock<mutex> lk(cli2mtx);
        while (!cli2) cli2mtxCv.wait(lk);
        try {
        if (!login(cli2)) {
            close(cli2);
            cli2 = 0;
            continue;
        }
        while (true) {
            char command[4096];
            bzero(command, 4096);
            prompt(cli2);
            int n = read(cli2, &command, 4095);
            if (n < 0 || n == 0) {
                close(cli2);
                cli2 = 0;
                break;
            }
            command[strlen(command) - 2] = 0;
            cmd(command, cli2);
        }
        } catch (exception e) {}
    }
}

void Console::cmd(string cmd, int cli) {
    auto parts = Helper::explode(cmd, ' ');
    if (parts.size() == 0) return;
    if (parts.at(0) == "sessions") {
        if (parts.size() == 1 || parts.at(1) == "all") {
            return cmd_sessions(cli, true, true);
        }
        if (parts.size() == 1 || parts.at(1) == "master") {
            return cmd_sessions(cli, true, false);
        }
        if (parts.size() == 1 || parts.at(1) == "slave") {
            return cmd_sessions(cli, false, true);
        }
    }
    if (parts.at(0) == "stop") {
        if (parts.size() > 1) {
            return cmd_stop(cli, parts.at(1));
        }
    }
    if (parts.at(0) == "gws") {
        return cmd_gws(cli);
    }
    if (parts.at(0) == "save") {
        return cmd_save(cli);
    }
    if (parts.at(0) == "stat") {
        return cmd_stat(cli);
    }
    if (parts.at(0) == "exit") {
        close(cli);
    }
    usage(cli);
}

void Console::usage(int sock) {
    string usage = "Usage\n" \
            "\t sessions [all|master|slave|] - список сессий\n"
            "\t stop <ip> - удалить сессию\n"
            "\t gws - список шлюзов\n"
            "\t save - сохранить кеш\n"
            "\t stat - статистика\n"
            "\t exit - exit\n";
    write(sock, usage.c_str(), usage.size() + 1);
}

void Console::prompt(int sock) {
    string prompt = "> ";
    write(sock, prompt.c_str(), prompt.size() + 1);
}

void Console::cmd_sessions(int cli, bool master, bool slave) {
    auto sessions = storage->getIpv4Sessions();
    char buf[4096];
    const char lineFmt[] = "%-16s%-16s%-16s%s\n";
    const char delim[] = "------------------------------------------------------\n";
    bzero(buf, sizeof(buf));
    snprintf(buf, sizeof(buf), lineFmt, "IP", "Gateway", "Interface", "Master");
    write(cli, buf, strlen(buf) + 1);
    write(cli, delim, strlen(delim) + 1);
    for (auto session: sessions) {
        if (!master) if (session.second.isMaster()) continue;
        if (!slave) if (!session.second.isMaster()) continue;
        string master = "no";
        if (session.second.isMaster()) master = "yes";
        snprintf(buf, sizeof(buf), lineFmt, session.second.dstAsString().c_str(),
                session.second.srsAsString().c_str(),
                session.second.ifName().c_str(),
                master.c_str());
        write(cli, buf, strlen(buf) + 1);
    }
}

void Console::cmd_stop(int cli, string ip) {
    storage->stop(ip);
}

void Console::cmd_gws(int cli) {
    auto gws = storage->getIpv4Gws();
    char buf[4096];
    const char lineFmt[] = "%s\n";
    const char delim[] = "----------------\n";
    bzero(buf, sizeof(buf));
    snprintf(buf, sizeof(buf), lineFmt, "IP/Mask");
    write(cli, buf, strlen(buf) + 1);
    for (auto gw: gws) {
        snprintf(buf, sizeof(buf), "%s/%d\n", gw.to_string().c_str(), gw.getMask());
        write(cli, buf, strlen(buf) + 1);
    }
}

void Console::cmd_save(int cli) {
    storage->save();
}

void Console::cmd_stat(int cli) {
    auto sessions = storage->getIpv4Sessions();
    int masters = 0;
    int slaves = 0;
    for (auto session: sessions) {
        if (session.second.isMaster()) masters++; else slaves++;
    }
    string msg = "Total sessions: " + to_string(sessions.size()) + " masters: " +
                 to_string(masters) + " slaves:" + to_string(slaves) + "\n";
    write(cli, msg.c_str(), msg.length() + 1);
}


