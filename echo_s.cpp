#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

// Log levels
int LOG_LEVEL = 0; // Default log level

void log_trace(const std::string &msg) {
    if (LOG_LEVEL >= 6) {
        std::cerr << "TRACE: " << msg << std::endl;
    }
}

void log_debug(const std::string &msg) {
    if (LOG_LEVEL >= 5) {
        std::cerr << "DEBUG: " << msg << std::endl;
    }
}

void log_info(const std::string &msg) {
    if (LOG_LEVEL >= 3) {
        std::cerr << "INFO: " << msg << std::endl;
    }
}

void log_warning(const std::string &msg) {
    if (LOG_LEVEL >= 2) {
        std::cerr << "WARNING: " << msg << std::endl;
    }
}

void log_error(const std::string &msg) {
    if (LOG_LEVEL >= 1) {
        std::cerr << "ERROR: " << msg << std::endl;
    }
}

void log_fatal(const std::string &msg) {
    if (LOG_LEVEL >= 0) {
        std::cerr << "FATAL: " << msg << std::endl;
    }
}

int processConnection(int sockFd) {
    char buffer[MAX_BUFFER_SIZE];
    bool keepGoing = true;
    bool quitProgram = false;

    while (keepGoing) {
        memset(buffer, 0, sizeof(buffer));
        log_trace("Waiting for data...");
        ssize_t bytesRead = read(sockFd, buffer, sizeof(buffer));

        if (bytesRead > 0) {
            log_trace("Received data, processing...");
            log_debug("Received: " + std::string(buffer));

            // Check for CLOSE or QUIT
            if (strncmp(buffer, "CLOSE", 5) == 0) {
                keepGoing = false;
                log_debug("Connection closed by client.");
                log_trace("CLOSE command received.");
            } else if (strncmp(buffer, "QUIT", 4) == 0) {
                keepGoing = false;
                quitProgram = true;
                log_debug("Server quitting.");
                log_trace("QUIT command received.");
            }

            // Echo data back to client only if it's not CLOSE/QUIT
            if (!keepGoing) continue;

            log_trace("Echoing data back to client.");
            write(sockFd, buffer, bytesRead);
        } else if (bytesRead == 0) {
            keepGoing = false; // Client closed connection
            log_trace("Client closed the connection.");
        }
    }

    return quitProgram;
}

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

    log_trace("Starting server setup...");

    // Create socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        log_fatal("Socket creation failed");
        exit(-1);
    }
    log_info("Socket created successfully.");
    log_trace("Socket FD: " + std::to_string(listenFd));

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
            std::cout << ("Using port: " + std::to_string(ntohs(servaddr.sin_port))) << msg << std::endl;
            log_info("Socket bound to port: " + std::to_string(ntohs(servaddr.sin_port)));
            log_trace("Socket binding succeeded.");
        } else {
            log_warning("Port binding failed, retrying...");
            servaddr.sin_port = htons(ntohs(servaddr.sin_port) + 1); // Try next port
        }
    }

    // Listen for connections
    if (listen(listenFd, 1) != 0) {
        log_fatal("Listening failed");
        exit(-1);
    }
    log_info("Server is listening...");
    log_trace("Socket now listening for connections.");

    // Accept and process connections
    bool quitProgram = false;
    while (!quitProgram) {
        log_trace("Waiting for a new connection...");
        int connFd = accept(listenFd, NULL, NULL);
        if (connFd < 0) {
            log_fatal("Connection acceptance failed");
            exit(-1);
        }
        log_info("Connection accepted.");
        log_trace("New connection on FD: " + std::to_string(connFd));

        // Process the connection
        quitProgram = processConnection(connFd);
        close(connFd);
        log_trace("Connection closed.");
    }

    close(listenFd);
    log_info("Server shut down.");
    return 0;
}
