# Compiler and compiler flags
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Executable names
CLIENT = twmailer-client
SERVER = twmailer-server

# Source and header files
CLIENT_SRC = twmailer-client.cpp
CLIENT_HDR = twmailer-client.h
SERVER_SRC = twmailer-server.cpp
SERVER_HDR = twmailer-server.h

# Build rules
all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_SRC) $(CLIENT_HDR)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(CLIENT_SRC)

$(SERVER): $(SERVER_SRC) $(SERVER_HDR)
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(SERVER_SRC)

# Clean rule
clean:
	rm -f $(CLIENT) $(SERVER)

# Phony targets
.PHONY: all clean
