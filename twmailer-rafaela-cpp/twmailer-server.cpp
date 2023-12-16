    #include "twmailer-server.h"

    #define BUF 1024 // Buffer size for receiving commands
    #define BACKLOG_SIZE 5 // Number of pending connections in the queue

    // Constructor: Initializes the server with the given port and mail spool directory
        Server::Server(int port, std::string mailSpoolDir) {
    Server::port = port;
    Server::mailSpoolDir = mailSpoolDir;

    // Hier wird das Mail-Spool-Verzeichnis erstellt
    if (!createDirectory(mailSpoolDir)) {
        std::cerr << "Fehler beim Erstellen des Mail-Spool-Verzeichnisses: " << mailSpoolDir << std::endl;
        exit(EXIT_FAILURE);
    }

    serverSocket = createServerSocket(); // Erstellt den Server-Socket
    bindServerSocket(); // Bindet den Server-Socket an den angegebenen Port
    listenForConnections(); // Beginnt mit dem Warten auf eingehende Verbindungen
}

    // Destructor: Close the server socket when the server object is destroyed
    Server::~Server() {
        close(serverSocket);
    }

    // Creates a server socket and returns its descriptor
    int Server::createServerSocket() {
        int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (socketDescriptor == -1) {
            perror("Error creating server socket");
            exit(EXIT_FAILURE);
        }
        return socketDescriptor;
    }

    // Binds the server socket to the specified port
    void Server::bindServerSocket() {
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            perror("Error binding socket");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
    }

    // Listens for incoming client connections
    void Server::listenForConnections() {
        if (listen(serverSocket, BACKLOG_SIZE) == -1) {
            perror("Error listening for connections");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
    }

    // Main loop to accept and handle client connections
    void Server::startListening() {
        std::cout << "Listening on port " << port << ":\n";
        while (true) {
            std::cout << "Waiting for client connection...\n";
            int clientSocket = acceptClientConnection(); // Accept a client connection
            handleClientConnection(clientSocket); // Handle the client connection
        }
    }

    // Accepts a client connection and returns its socket descriptor
    int Server::acceptClientConnection() {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Error accepting connection");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
        return clientSocket;
    }

    // Handles the communication with a connected client
    void Server::handleClientConnection(int clientSocket) {
        std::cout << "Client connected. Handling connection...\n";

        if (!sendWelcomeMessage(clientSocket)) {
            return;
        }

        while (true) {
            std::string command = receiveCommand(clientSocket); // Receive a command from the client
            if (command.empty()) {
                break; // Break the loop if no command is received or on error
            }

            if (!processCommand(clientSocket, command)) {
                break; // Break the loop if processing results in disconnection
            }
        }

        closeClientConnection(clientSocket); // Close the client connection
    }

    // Sends a welcome message to the connected client
    bool Server::sendWelcomeMessage(int clientSocket) {
        const char* welcomeMessage = "Please choose your command. SEND, LIST, READ, QUIT";
        if (send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0) == -1) {
            perror("Send error");
            return false;
        }
        return true;
    }

    // Receives a command from the client and returns it as a string
    std::string Server::receiveCommand(int clientSocket) {
        char buffer[BUF];
        memset(buffer, 0, BUF); // Clear the buffer

        ssize_t bytesReceived = recv(clientSocket, buffer, BUF - 1, 0);
        if (bytesReceived <= 0) {
            handleReceiveError(bytesReceived);
            return "";
        }

        buffer[bytesReceived] = '\0'; // Null-terminate the received string
        std::string command(buffer);
        replaceBackslashNWithNewline(command); // Convert \n sequences to actual newlines
        return command;
    }

    // Handles errors that occur during the receive operation
    void Server::handleReceiveError(ssize_t bytesReceived) {
        if (bytesReceived == 0) {
            std::cout << "Client disconnected.\n";
        } else {
            perror("Receive error");
        }
    }

    // Replaces the literal "\\n" sequences in a string with actual newline characters
    void Server::replaceBackslashNWithNewline(std::string &str) {
        size_t index = 0;
        while (true) {
            index = str.find("\\n", index);
            if (index == std::string::npos) break;
            str.replace(index, 2, "\n");
            index += 1;
        }
    }

    // Processes a received command and performs the corresponding action
    bool Server::processCommand(int clientSocket, const std::string& command) {
        std::istringstream iss(command);
        std::string commandName;
        std::getline(iss, commandName);

        if (commandName == "SEND") {
            std::cout << "SEND command received:\n";
            processSendCommand(clientSocket, command);
        } else if (commandName == "LIST") {
            std::cout << "LIST command received:\n";
            processListCommand(clientSocket, command);
        } else if (commandName == "READ") {
            std::cout << "READ command received:\n";
            processReadCommand(clientSocket, command);
        } else if (commandName == "DEL") {
            std::cout << "DEL command received:\n";
            processDelCommand(clientSocket, command);
        } else if (commandName == "QUIT") {
            std::cout << "QUIT command received. Ending connection.\n";
            return false;
        } else {
            std::cout << "Unknown command received: " << commandName << "\n";
        }
        return true;
    }

bool Server::processSendCommand(int clientSocket, const std::string &command) {
    // Parsing the command into components using a string stream
    std::istringstream iss(command);
    std::string line, sender, receiver, subject, text;

    // Skip the first line which is the command name "SEND"
    std::getline(iss, line);

    // Extract sender, receiver, and subject from the following lines
    std::getline(iss, sender);
    std::getline(iss, receiver);
    std::getline(iss, subject);
    std::getline(iss, text);

    // Compose the message body until a single dot line is encountered
    message = "Sender: " + sender + "\nReceiver: " + receiver + "\nSubject: " + subject + "\nMessage: " + text + "\n\n";
    while (std::getline(iss, line) && line != ".") {
        message += line + "\n";
    }

    // If there is content after the final dot, it indicates an invalid message format
    if (std::getline(iss, line) && !line.empty()) {
        send(clientSocket, "ERR Invalid message format\n", 27, 0);
        return false;
    }

    std::string receiverDir = mailSpoolDir + "/" + receiver;

    if (receiver.empty()) {
        std::cout << "Invalid receiver name\n";
        send(clientSocket, "ERR\n", 4, 0);
        return false;
    }

    // Überprüfe, ob das Verzeichnis bereits existiert
    if (!directoryExists(receiverDir)) {
        std::cout << "Directory does not exist. Creating: " << receiverDir << std::endl;

        // Versuche, das Verzeichnis zu erstellen
        if (!createDirectory(receiverDir)) {
            std::cout << "Failed to create directory: " << receiverDir << std::endl;
            send(clientSocket, "ERR\n", 4, 0);
            return false;
        }
    }

    std::string filename = generateMessageFilename(receiverDir, sender, receiver);
    saveMessage(filename, message);

    send(clientSocket, "OK\n", 3, 0);
    return true;
}




bool Server::createDirectory(const std::string& path) {
    struct stat st = {};
    if (path.empty()) {
        std::cerr << "Empty path provided to createDirectory function." << std::endl;
        return false;
    }

    // Check if the directory already exists
    if (stat(path.c_str(), &st) != 0 || !(st.st_mode & S_IFDIR)) {
        // Attempt to create the directory
        if (mkdir(path.c_str(), 0777) != 0) {
            perror("mkdir");
            std::cout << "Failed to create directory: " << path << std::endl;
            return false;
        } else {
            std::cout << "Directory created: " << path << std::endl;
        }
    } else {
        std::cout << "Directory already exists: " << path << std::endl;
    }

    return true;
}

    bool Server::directoryExists(const std::string& path) {
        struct stat st = {};
        return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
    }

std::string Server::generateMessageFilename(const std::string& dir, const std::string& sender, const std::string& receiver) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // Format the filename with directory, sender, and timestamp
    std::string filename = dir + "/" + sender + "_msg_" + std::to_string(timestamp) + ".txt";
    return filename;
}

    // Saves a message to a file with the given filename
    void Server::saveMessage(const std::string& filename, const std::string& message) {
        std::ofstream outFile(filename);
        if (outFile.is_open()) {
            outFile << message;
            outFile.close();
        } else {
            std::cerr << "Unable to open file: " << filename << std::endl;
        }
    }

    // Processes the "LIST" command from the client
   void Server::processListCommand(int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string commandName, username;

    // Extract the command name and username from the command string
    std::getline(iss, commandName); // This should be "LIST"
    std::getline(iss, username);

    // Construct the directory path where the user's messages are stored
    std::string userDir = mailSpoolDir + "/" + username;

    // Check if the user's directory exists
    if (!directoryExists(userDir)) {
        send(clientSocket, "ERR User has no inbox\n", 24, 0);
        return;
    }

    // Retrieve the list of files (messages) in the user's directory
    std::vector<std::string> files = listFilesInDirectory(userDir);

    // Start building the response with the number of messages
    std::string response = std::to_string(files.size()) + " Mails found in Inbox of " + username + "\n";

    int messageNumber = 1; // Start message numbering from 1

    // Loop through each message file
    for (const std::string& file : files) {
        // Extract the subject line from each message
        std::string subject = extractSubjectFromMessage(userDir + "/" + file);

        // Append the message number and subject to the response
        response += std::to_string(messageNumber++) + ". " + subject + "\n";
    }

    // Send the compiled response back to the client
    send(clientSocket, response.c_str(), response.length(), 0);
}

    // Lists files in the given directory and returns their names
    std::vector<std::string> Server::listFilesInDirectory(const std::string& directoryPath) {
        std::vector<std::string> files;
        DIR* dir = opendir(directoryPath.c_str());
        struct dirent* entry;

        if (dir != nullptr) {
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_type == DT_REG) { // Only include regular files
                    files.push_back(entry->d_name);
                }
            }
            closedir(dir);
        }
        return files;
    }

    // Extracts and returns the subject line from a message file.
    std::string Server::extractSubjectFromMessage(const std::string& filePath) {
        std::ifstream file(filePath);
        std::string line;
        std::string subjectPrefix = "Subject: ";

        if (file.is_open()) {
            while (getline(file, line)) {
                // Check if the line starts with "Subject: "
                if (line.find(subjectPrefix) == 0) {
                    return line.substr(subjectPrefix.length());
                }
            }
        }
        return "";
    }

    // Processes the READ command to send the content of a specific message to the client
    void Server::processReadCommand(int clientSocket, const std::string& command) {
        std::istringstream iss(command);
        std::string commandName, username, messageNumberStr;
        std::getline(iss, commandName); // This should be "READ"
        std::getline(iss, username);
        std::getline(iss, messageNumberStr);
        int messageNumber = std::stoi(messageNumberStr); // Convert string to int

        std::string userDir = mailSpoolDir + "/" + username;
        std::vector<std::string> files = listFilesInDirectory(userDir);

        // Validate message number
    if (messageNumber < 1 || messageNumber > static_cast<ssize_t>(files.size()))  {
            send(clientSocket, "ERR\n", 4, 0); // Inform client of invalid message number
            return;
        }

        // Construct the file path and read the message
        std::string filename = userDir + "/" + files[messageNumber - 1];
        std::string messageContent = readFileContent(filename);

        // Send the message content or an error response
        if (!messageContent.empty()) {
            std::string response = "OK\n" + messageContent + "\n";
            send(clientSocket, response.c_str(), response.length(), 0);
        } else {
            send(clientSocket, "ERR\n", 4, 0); // File reading error
        }
    }

    // Reads and returns the content of a file given its path
    std::string Server::readFileContent(const std::string& filePath) {
        std::ifstream file(filePath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }

    // Processes the DEL command to delete a specific message for a user
    void Server::processDelCommand(int clientSocket, const std::string& command) {
        std::istringstream iss(command);
        std::string commandName, username, messageNumberStr;
        std::getline(iss, commandName); // This should be "DEL"
        std::getline(iss, username);
        std::getline(iss, messageNumberStr);
        int messageNumber = std::stoi(messageNumberStr); // Convert string to int

        std::string userDir = mailSpoolDir + "/" + username;
        std::vector<std::string> files = listFilesInDirectory(userDir);

        // Validate message number
        if (messageNumber < 1 || messageNumber > static_cast<ssize_t>(files.size())) { 
            send(clientSocket, "ERR\n", 4, 0); // Inform client of invalid message number
            return;
        }

        // Determine the file to delete and attempt deletion
        std::string fileToDelete = userDir + "/" + files[messageNumber - 1];
        if (remove(fileToDelete.c_str()) != 0) {
            perror("Error deleting file");
            send(clientSocket, "ERR\n", 4, 0); // Notify client of deletion error
        } else {
            send(clientSocket, "OK\n", 3, 0); // Confirm successful deletion
        }
    }

    // Closes the client's connection
    void Server::closeClientConnection(int clientSocket) {
        close(clientSocket);
        std::cout << "Client connection closed.\n";
    }

    int main(int argc, char *argv[]) {

        // Display correct usage for the Client
        if (argc != 3) {
            std::cerr << "Usage: ./twmailer-server <port> <mail-spool-directoryname>\n";
            return EXIT_FAILURE;
        }

        // Extract the port and mail spool directory
        int port = std::stoi(argv[1]);
        std::string mailSpoolDir = argv[2];

        // Create mail server
        Server mailServer(port, mailSpoolDir);

        // Start listening for client connections
        mailServer.startListening();

        return EXIT_SUCCESS;
    }
