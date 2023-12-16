#ifndef SERVER_H
#define SERVER_H
#pragma once

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fstream>
#include <chrono>
#include <vector>
#include <dirent.h>

class Server {
public:
    Server(int port, std::string mailSpoolDir);
    ~Server();

    void startListening(); // Start listening for client connections

private:
    int createServerSocket();
    void bindServerSocket();
    void listenForConnections();
    int acceptClientConnection();
    void handleClientConnection(int clientSocket);
    void closeClientConnection(int clientSocket);
    bool processCommand(int clientSocket, const std::string& command);
    bool processSendCommand(int clientSocket, const std::string &command);
    void processListCommand(int clientSocket, const std::string& command);
    void processReadCommand(int clientSocket, const std::string& command);
    void processDelCommand(int clientSocket, const std::string& command);
    bool createDirectory(const std::string& path);
    std::string generateMessageFilename(const std::string& dir, const std::string& sender, const std::string& receiver);
    void saveMessage(const std::string& filename, const std::string& message);
    std::vector<std::string> listFilesInDirectory(const std::string& directoryPath);
    std::string extractSubjectFromMessage(const std::string& filePath);
    std::string readFileContent(const std::string& filePath);
    void replaceBackslashNWithNewline(std::string &str);
    bool sendWelcomeMessage(int clientSocket);
    std::string receiveCommand(int clientSocket);
    void handleReceiveError(ssize_t bytesReceived);
    bool directoryExists(const std::string& path);


private:
    int serverSocket;
    int port;
    std::string mailSpoolDir;
    std::string sender;
    std::string receiver;
    std::string subject;
    std::string message;
};

#endif // SERVER_H
