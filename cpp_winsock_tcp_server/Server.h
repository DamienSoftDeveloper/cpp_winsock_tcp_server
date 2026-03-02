#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>   
#include <vector>
#include <mutex>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

class Server
{
public:
    Server();
    ~Server();

    bool Initialize();
    void Run();

private:
    void HandleClient(SOCKET clientSocket);
    void Broadcast(const string& message);

    WSADATA wsaData;
    SOCKET serverSocket;
    vector<SOCKET> clients;
    mutex clientsMutex;

};