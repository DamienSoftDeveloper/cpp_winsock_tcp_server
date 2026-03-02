// cpp_winsock_tcp_server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Server.h"

using namespace std;

int main()
{
    Server server;

    if (!server.Initialize())
    {
        return -1;
    }

    cout << "Server initialization complete." << endl;

    server.Run();

    return 0;
}