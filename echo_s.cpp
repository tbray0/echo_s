// **************************************************************************************
// * Echo Strings (echo_s.cc)
// * -- Accepts TCP connections and then echos back each string sent.
// **************************************************************************************
#include "echo_s.h"
#include "logging.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <filesystem>
#include <cstring>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024

// **************************************************************************************
// * processConnection()
// * - Handles reading the line from the network and sending it back to the client.
// * - Returns true if the client sends "QUIT" command, false if the client sends "CLOSE".
// **************************************************************************************
int processConnection(int sockFd) {
    char buffer[MAX_BUFFER_SIZE];
    bool keepGoing = true;
    bool quitProgram = false;

    while (keepGoing) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = read(sockFd, buffer, sizeof(buffer));

        if (bytesRead > 0) {
            // Debugging: Received data
            DEBUG std::cout << "Received: " << buffer << ENDL;

            // Check for CLOSE or QUIT
            if (strncmp(buffer, "CLOSE", 5) == 0) {
                keepGoing = false;
                DEBUG std::cout << "Connection closed by client." << ENDL;
            } else if (strncmp(buffer, "QUIT", 4) == 0) {
                keepGoing = false;
                quitProgram = true;
                DEBUG std::cout << "Server quitting." << ENDL;
            }

            // Echo data back to client
            write(sockFd, buffer, bytesRead);
        } else if (bytesRead == 0) {
            keepGoing = false; // Client closed connection
        }
    }

    return quitProgram;
}
    


// **************************************************************************************
// * main()
// * - Sets up the sockets and accepts new connection until processConnection() returns 1
// **************************************************************************************

int main(int argc, char *argv[]) {
    // Parse command line arguments for debug level
    int opt = 0;
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd':
                LOG_LEVEL = std::stoi(optarg);
                break;
            default:
                std::cout << "Usage: " << argv[0] << " -d <num>" << std::endl;
                exit(-1);
        }
    }

    // Create socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        FATAL std::cerr << "Socket creation failed" << ENDL;
        exit(-1);
    }
    INFO std::cout << "Socket created successfully." << ENDL;

    // Bind the socket to an address
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8080); // Default port

    bool bindSuccessful = false;
    while (!bindSuccessful) {
        if (bind(listenFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == 0) {
            bindSuccessful = true;
            INFO std::cout << "Socket bound to port: " << ntohs(servaddr.sin_port) << ENDL;
        } else {
            WARNING std::cerr << "Port binding failed, retrying..." << ENDL;
            servaddr.sin_port = htons(ntohs(servaddr.sin_port) + 1); // Try next port
        }
    }

    // Listen for connections
    if (listen(listenFd, 1) != 0) {
        FATAL std::cerr << "Listening failed" << ENDL;
        exit(-1);
    }
    INFO std::cout << "Server is listening..." << ENDL;

    // Accept and process connections
    bool quitProgram = false;
    while (!quitProgram) {
        int connFd = accept(listenFd, NULL, NULL);
        if (connFd < 0) {
            FATAL std::cerr << "Connection acceptance failed" << ENDL;
            exit(-1);
        }
        INFO std::cout << "Connection accepted." << ENDL;

        // Process the connection
        quitProgram = processConnection(connFd);
        close(connFd);
    }

    close(listenFd);
    INFO std::cout << "Server shut down." << ENDL;
    return 0;
}
