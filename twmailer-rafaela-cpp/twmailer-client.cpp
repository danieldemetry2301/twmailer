#include "twmailer-client.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <cctype>

#define BUF 1024

// Sets the IP and port for the server, creates a socket, connects to the server, and handles communication
Client::Client(std::string ip, int port)
{
    Client::ip = ip;
    Client::port = port;
    clientSocket = createClientSocket();
    connectToServer();
    handleCommunication();
}

// Closes the connection when the client object is destroyed
Client::~Client()
{
    closeConnection();
}

// Creates a socket for the client and returns the socket descriptor
int Client::createClientSocket()
{
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    // Check if the socket creation was successful
    if (socketDescriptor == -1)
    {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }
    clientSocket = socketDescriptor;
    return socketDescriptor;
}

// Connects to the server
void Client::connectToServer()
{
    struct sockaddr_in address;           // Define a sockaddr_in struct for the connection.
    memset(&address, 0, sizeof(address)); // Initialize the struct to zero.
    address.sin_family = AF_INET;         // Set the address family to AF_INET (IPv4).
    address.sin_port = htons(port);       // Set the port number, converting it to network byte order.

    // Convert the IP address from std::string to const char* and store it in the address struct.
    if (inet_aton(ip.c_str(), &address.sin_addr) == 0)
    {
        std::cerr << "Invalid IP address format\n"; // Print an error if the IP format is invalid.
        exit(EXIT_FAILURE);                         // Exit the program with a failure status.
    }

    // Attempt to connect the client socket to the server's address and port.
    if (connect(clientSocket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("Connect error"); // Print an error message if connection fails.
        closeConnection();       // Close the connection in case of failure.
        exit(EXIT_FAILURE);      // Exit the program with a failure status.
    }
}

// Handles communication wiht the server
void Client::handleCommunication()
{
    receiveWelcomeMessageFromServer();

    char buffer[BUF];

    while (true)
    {
        std::cout << ">> ";
        if (fgets(buffer, BUF, stdin) == nullptr)
        {
            continue;
        }

        buffer[strcspn(buffer, "\r\n")] = 0;

        std::string command = buffer;
        if (!isValidCommand(command))
        {
            continue;
        }

        // SEND Command
        if (command == "SEND")
        {
            std::cout << "Enter the Sender (max 8 characters): ";
            std::string sender;
            std::getline(std::cin, sender);

            while (!isValidName(sender))
            {
                std::cout << "Please enter a valid Sender name.\n";
                std::cout << "Enter the Sender (max 8 characters): ";
                std::getline(std::cin, sender);
            }

            std::cout << "Enter the Receiver (max 8 characters): ";
            std::string receiver;
            std::getline(std::cin, receiver);

            while (!isValidName(receiver))
            {
                std::cout << "Please enter a valid Receiver name.\n";
                std::cout << "Enter the Receiver (max 8 characters): ";
                std::getline(std::cin, receiver);
            }

            std::cout << "Enter the Subject: ";
            std::string subject;
            std::getline(std::cin, subject);

            std::cout << "Enter the Message (type '.' on a new line to finish):\n";
            std::string message;
            std::string line;

            do
            {
                std::getline(std::cin, line);
                if (line != ".")
                {
                    message += line + "\n";
                }
            } while (line != ".");

            // Send the command, sender, and receiver to the server
            std::string fullCommand = command + "\n" + sender + "\n" + receiver + "\n" + subject + "\n" + message + "\n";
            if (send(clientSocket, fullCommand.c_str(), fullCommand.length(), 0) == -1)
            {
                std::cout << "Error!";
                break;
            }
            else
            {
                std::cout << "Message was sent!\n";
            }
        }

        // LIST Command
        if (command == "LIST")
        {
            std::cout << "Enter the Username you want to List the Inbox: ";
            std::string inboxuser;
            std::getline(std::cin, inboxuser);

            while (!isValidName(inboxuser))
            {
                std::cout << "Please enter a valid Username\n";
                std::cout << "Enter the Username you want to List the Inbox: ";
                std::getline(std::cin, inboxuser);
            }

            std::string fullCommand = command + "\n" + inboxuser + "\n";
            if (send(clientSocket, fullCommand.c_str(), fullCommand.length(), 0) == -1)
            {
                perror("Send error");
                break;
            }
        }

        // READ Command
        if (command == "READ")
        {
            std::cout << "Enter the Username: ";
            std::string username;
            std::getline(std::cin, username);

            while (!isValidName(username))
            {
                std::cout << "Please enter a valid Username\n";
                std::cout << "Enter the Username: ";
                std::getline(std::cin, username);
            }

            std::cout << "Enter the Message Number: ";
            std::string messageNumber;
            std::getline(std::cin, messageNumber);

            std::string fullCommand = command + "\n" + username + "\n" + messageNumber + "\n";
            if (send(clientSocket, fullCommand.c_str(), fullCommand.length(), 0) == -1)
            {
                perror("Send error");
                break;
            }
        }

        // DEL Command
        if (command == "DEL")
        {
            std::cout << "Enter the Username: ";
            std::string username;
            std::getline(std::cin, username);

            while (!isValidName(username))
            {
                std::cout << "Please enter a valid Username\n";
                std::cout << "Enter the Username: ";
                std::getline(std::cin, username);
            }

            std::cout << "Enter the Message Number: ";
            std::string messageNumber;
            std::getline(std::cin, messageNumber);

            // Send the command, username, and messageNumber to the server
            std::string fullCommand = command + "\n" + username + "\n" + messageNumber + "\n";
            if (send(clientSocket, fullCommand.c_str(), fullCommand.length(), 0) == -1)
            {
                perror("Send error");
                break;
            }
        }

        // QUIT Command
        if (command == "QUIT")
        {
            // Send the command, username, and messageNumber to the server
            std::string fullCommand = command;
            if (send(clientSocket, fullCommand.c_str(), fullCommand.length(), 0) == -1)
            {
                perror("Send error");
                break;
            }
        }

        std::string serverResponse;
        while (true)
        {
            ssize_t size = recv(clientSocket, buffer, BUF - 1, 0);
            if (size <= 0)
            {
                std::cerr << "Server closed remote socket or recv error" << std::endl;
                return;
            }
            buffer[size] = '\0';
            serverResponse += buffer;
            if (serverResponse.find("\n") != std::string::npos)
            {
                break;
            }
        }

        std::cout << "<< " << serverResponse;
    }
}

bool Client::isValidName(const std::string &name)
{
    if (name.length() > 8)
    {
        std::cout << "Max. 8 Characters allowed!\n";
        return false;
    }

    for (char c : name)
    {
        if (!islower(c) && !isdigit(c))
        {
            std::cout << "Invalid name. Please use a name with characters a-z (lowercase) and digits 0-9. No special characters allowed!\n";
            return false;
        }
    }
    return true;
}

// checks if the received command is valid
bool Client::isValidCommand(const std::string &command)
{
    // Split the command into parts using space as delimiter
    std::vector<std::string> parts;
    std::string part;
    std::istringstream iss(command);
    while (iss >> part)
    {
        parts.push_back(part);
    }

    if (parts.empty())
    {
        printUsage();
        return false;
    }

    std::string commandName = parts[0];
    if (parts.size() != 1)
    {
        printUsage();
        return false;
    }
    if (commandName == "SEND" || commandName == "LIST" || commandName == "READ" || commandName == "DEL" || commandName == "QUIT")
    {
        return true;
    }
    else
    {
        printUsage();
        return false;
    }
    return false;
}

// prints the correct usage for the program
void Client::printUsage()
{
    std::cerr << "Invalid command or format. Valid commands are: SEND, LIST, READ, DEL, QUIT";
}

// closes the client connection
void Client::closeConnection()
{
    if (clientSocket != -1)
    {
        if (shutdown(clientSocket, SHUT_RDWR) == -1)
        {
            perror("shutdown clientSocket");
        }
        if (close(clientSocket) == -1)
        {
            perror("close clientSocket");
        }
        clientSocket = -1;
    }
}

// receives the initial welcome message from the server to test communication
void Client::receiveWelcomeMessageFromServer()
{
    char buffer[1024];
    std::string response;

    // Clear the buffer
    memset(buffer, 0, sizeof(buffer));

    // Receive data from the server
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0)
    {
        // Handle errors or closed connection
        if (bytesReceived == 0)
        {
            std::cout << "Server closed the connection.\n";
        }
        else
        {
            perror("Receive error");
        }
    }

    // Convert the received data to a std::string
    response = std::string(buffer);
    // print server's response
    std::cout << "\n<< " << response << "\n";
}

int main(int argc, char *argv[])
{
    // Display correct usage for the Client
    if (argc != 3)
    {
        std::cerr << "Usage: ./twmailer-client <ip> <port>\n";
        return EXIT_FAILURE;
    }

    // Extract IP address and port
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);

    // Create client
    Client client(ip, port);

    return EXIT_SUCCESS;
}
