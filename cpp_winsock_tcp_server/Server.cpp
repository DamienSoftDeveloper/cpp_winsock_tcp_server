#include "Server.h"
#include <algorithm>

Server::Server()
{
    serverSocket = INVALID_SOCKET;
}

Server::~Server()
{
    if (serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
    }

    WSACleanup();
}

bool Server::Initialize()
{
    WORD version = MAKEWORD(2, 2);

    int result = WSAStartup(version, &wsaData);

    if (result != 0)
    {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed with error: "
            << WSAGetLastError() << std::endl;

        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(54000);

    if (bind(serverSocket,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed with error: "
            << WSAGetLastError() << std::endl;

        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed with error: "
            << WSAGetLastError() << std::endl;

        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Winsock initialized successfully.\n";
    std::cout << "Socket created successfully.\n";
    std::cout << "Bind successful.\n";
    std::cout << "Server is now listening on port 54000...\n";

    return true;
}

void Server::Run()
{
    std::cout << "Server ready. Waiting for clients...\n";

    while (true)
    {
        sockaddr_in clientAddr{};
        int clientSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(
            serverSocket,
            (sockaddr*)&clientAddr,
            &clientSize
        );

        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Accept failed: "
                << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "Client connected.\n";

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        std::thread clientThread(
            &Server::HandleClient,
            this,
            clientSocket
        );

        clientThread.detach();
    }
}

void Server::HandleClient(SOCKET clientSocket)
{
    char buffer[4096];
    int bytesReceived;
    std::string username = "Unknown";

    while (true)
    {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0)
        {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string message(buffer, bytesReceived);

        // Remove CRLF
        message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
        message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());

        std::cout << "Received: " << message << std::endl;

        if (message.rfind("JOIN ", 0) == 0)
        {
            username = message.substr(5);
            Broadcast("SERVER: " + username + " joined the lobby.\n");
        }
        else if (message.rfind("MSG ", 0) == 0)
        {
            std::string content = message.substr(4);
            Broadcast(username + ": " + content + "\n");
        }
        else if (message == "QUIT")
        {
            Broadcast("SERVER: " + username + " left the lobby.\n");
            break;
        }
        else
        {
            send(clientSocket, "Unknown command\n", 16, 0);
        }
    }

    // Remove client from list safely
    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        clients.erase(
            std::remove(clients.begin(), clients.end(), clientSocket),
            clients.end()
        );
    }

    closesocket(clientSocket);
}

void Server::Broadcast(const std::string& message)
{
    std::vector<SOCKET> clientsCopy;

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientsCopy = clients;
    }

    for (SOCKET client : clientsCopy)
    {
        send(client, message.c_str(), static_cast<int>(message.size()), 0);
    }
}