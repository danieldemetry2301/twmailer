#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <sstream>

class Client {
public:
    Client(std::string ip, int port);
    ~Client();

private:
    int createClientSocket();
    void connectToServer();
    void handleCommunication();
    void closeConnection();
    void receiveWelcomeMessageFromServer();
    bool isValidCommand(const std::string& command);
    void printUsage();
    bool isValidName(const std::string& name);
    


private:
    int clientSocket;
    std::string ip;
    int port;
    

};

#endif
