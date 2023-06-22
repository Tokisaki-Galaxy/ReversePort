// ReversePortClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define BUFSIZE 4096          // 缓冲区大小
#define SERVERIP "192.168.0.1" // 服务器IP地址
#define SERVERPORT 2000       // 服务器开放的端口号
#define LOCALPORT1 1225       // 本地端口1
#define LOCALPORT2 1224       // 本地端口2

void forwardData(SOCKET s, SOCKET c) {
    int ret;
    char buf[BUFSIZE];
    while (1) {
        ret = recv(s, buf, BUFSIZE, 0);
        if (ret <= 0) break;
        send(c, buf, ret, 0);
    }
    closesocket(s);
    closesocket(c);
}

int main() {
    WSADATA wsaData;
    SOCKET s, c1, c2;
    SOCKADDR_IN saddr, caddr;
    int len = sizeof(caddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket failed\n");
        WSACleanup();
        return 1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVERPORT);
    saddr.sin_addr.s_addr = inet_addr(SERVERIP);

    if (connect(s, (SOCKADDR*)&saddr, sizeof(saddr)) == SOCKET_ERROR) {
        fprintf(stderr, "connect failed\n");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // 此处向服务器发送映射请求
    // ...

    // 监听本地端口1
    c1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c1 == INVALID_SOCKET) {
        fprintf(stderr, "socket failed\n");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    memset(&caddr, 0, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(LOCALPORT1);
    caddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(c1, (SOCKADDR*)&caddr, sizeof(caddr)) != 0) {
        fprintf(stderr, "bind failed\n");
        closesocket(c1);
        WSACleanup();
        return 1;
    }

    if (listen(c1, 10) != 0) {
        fprintf(stderr, "listen failed\n");
        closesocket(c1);
        WSACleanup();
        return 1;
    }

    // 监听本地端口2
    c2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c2 == INVALID_SOCKET) {
        fprintf(stderr, "socket failed\n");
        closesocket(c1);
        WSACleanup();
        return 1;
    }

    memset(&caddr, 0, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(LOCALPORT2);
    caddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(c2, (SOCKADDR*)&caddr, sizeof(caddr)) != 0) {
        fprintf(stderr, "bind failed\n");
        closesocket(c2);
        closesocket(c1);
        WSACleanup();
        return 1;
    }

    if (listen(c2, 10) != 0) {
        fprintf(stderr, "listen failed\n");
        closesocket(c2);
        closesocket(c1);
        WSACleanup();
        return 1;
    }

    while (1) {
        SOCKET s1, s2;
        SOCKADDR_IN saddr1, saddr2;
        std::thread t1, t2;

        s1 = accept(c1, (SOCKADDR*)&saddr1, &len);
        if (s1 == INVALID_SOCKET) continue;
        s2 = accept(c2, (SOCKADDR*)&saddr2, &len);
        if (s2 == INVALID_SOCKET) {
            closesocket(s1);
            continue;
        }

        t1 = std::thread(forwardData, s2, s1);
        t1.detach();
        t2 = std::thread(forwardData, s1, s2);
        t2.detach();
    }

    closesocket(c1);
    closesocket(c2);
    closesocket(s);
    WSACleanup();
    return 0;
}
