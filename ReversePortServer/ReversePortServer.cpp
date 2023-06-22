// ReversePortServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define BUFSIZE 4096
#define CONFIGFILE "config.txt"
#define MAXCLIENTS 10

struct TunnelConfig {
    int localPort;
    int remotePort;
};

std::vector<TunnelConfig> g_tunnelConfig;

void handleClient(SOCKET c) {
    SOCKET s;
    char buf[BUFSIZE];
    int ret;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket failed\n");
        return;
    }

    memset(buf, 0, sizeof(buf));
    ret = recv(c, buf, BUFSIZE, 0);
    if (ret <= 0 || strncmp(buf, "TUNNEL ", 7) != 0) {
        closesocket(c);
        closesocket(s);
        return;
    }

    int rport, lport;
    sscanf_s(buf + 7, "%d %d", &rport, &lport);

    SOCKADDR_IN saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(rport);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (SOCKADDR*)&saddr, sizeof(saddr)) != 0) {
        fprintf(stderr, "bind failed\n");
        closesocket(s);
        closesocket(c);
        return;
    }

    if (listen(s, 10) != 0) {
        fprintf(stderr, "listen failed\n");
        closesocket(s);
        closesocket(c);
        return;
    }

    while (1) {
        SOCKET cs;
        SOCKADDR_IN caddr;
        int len = sizeof(caddr);

        cs = accept(s, (SOCKADDR*)&caddr, &len);
        if (cs == INVALID_SOCKET) continue;

        std::thread t(handleTunnel, c, cs, lport);
        t.detach();
    }
}

void handleTunnel(SOCKET s, SOCKET c, int lport) {
    char buf[BUFSIZE];
    int ret;

    memset(buf, 0, sizeof(buf));
    sprintf_s(buf, "CONNECT %d\n", lport);

    ret = send(s, buf, (int)strlen(buf), 0);
    if (ret <= 0) return;

    while (1) {
        ret = recv(c, buf, BUFSIZE, 0);
        if (ret <= 0) break;
        send(s, buf, ret, 0);
    }

    closesocket(s);
    closesocket(c);
}

void loadConfig() {
    g_tunnelConfig.clear();

    FILE* fp;
    fopen_s(&fp, CONFIGFILE, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open config file.\n");
        return;
    }

    char buf[100];
    while (fgets(buf, 100, fp) != NULL) {
        TunnelConfig conf;
        if (sscanf_s(buf, "%d %d", &conf.localPort, &conf.remotePort) != 2) continue;
        g_tunnelConfig.push_back(conf);
    }
}

int main() {
    WSADATA wsaData;
    SOCKET s, c;
    SOCKADDR_IN saddr, caddr;
    int len = sizeof(caddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    loadConfig();

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket failed\n");
        WSACleanup();
        return 1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2000);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (SOCKADDR*)&saddr, sizeof(saddr)) != 0) {
        fprintf(stderr, "bind failed\n");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    if (listen(s, 10) != 0) {
        fprintf(stderr, "listen failed\n");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    while (1) {
        c = accept(s, (SOCKADDR*)&caddr, &len);
        if (c == INVALID_SOCKET) continue;

        std::thread t(handleClient, c);
        t.detach();
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
