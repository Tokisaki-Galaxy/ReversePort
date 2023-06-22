// test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

const short localPorts[] = { 1224, 1225 };

void handleConnection(SOCKET clientSocket, short port)
{
    char recvbuf[1024];
    ZeroMemory(recvbuf, sizeof(recvbuf));
    int iRecvResult = recv(clientSocket, recvbuf, sizeof(recvbuf), 0);

    if (iRecvResult > 0)
    {
        std::cout << "Received data from port " << port << ": " << recvbuf << std::endl;
    }

    closesocket(clientSocket);
}

void listenPort(short port)
{
    sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "socket failed: " << WSAGetLastError() << std::endl;
        return;
    }

    int iResult = bind(listenSocket, (const sockaddr*)&addr, sizeof(addr));
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "accept failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            return;
        }

        std::thread th(handleConnection, clientSocket, port);
        th.detach();
    }

    closesocket(listenSocket);
}

int main()
{
    // 初始化Winsock库
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
    // 创建监听线程
    std::thread th_1(listenPort, localPorts[0]);
    std::thread th_2(listenPort, localPorts[1]);

    // 等待线程结束
    th_1.join();
    th_2.join();

    // 关闭Winsock库
    WSACleanup();

    return 0;
}