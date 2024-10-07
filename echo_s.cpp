#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <regex>
#include <fstream>

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

void log_error(const std::string &msg) {
    std::cerr << "ERROR: " << msg << std::endl;
}

int processConnection(int sockfd);
int readRequest(int socketFD, std::string *filename);
void sendLine(int socketFD, const std::string &stringToSend);
void send404(int socketFD);
void send400(int socketFD);
void send200(int socketFD, const std::string &filename);
std::string getContentType(const std::string &filename);

int main(int argc, char *argv[]) {
    int port = 8080; // Default port
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    // Create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        log_error("Failed to create socket.");
        return 1;
    }

    // Bind socket to IP/Port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_error("Failed to bind socket.");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        log_error("Failed to listen on socket.");
        return 1;
    }

    std::cout << "Server is listening on port: " << port << std::endl;

    // Main loop to accept and process connections
    while (true) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            log_error("Failed to accept connection.");
            continue;
        }
        processConnection(client_socket);
        close(client_socket);
    }

    close(server_fd);
    return 0;
}

// Process connection and respond accordingly
int processConnection(int sockfd) {
    std::string filename;
    int returnCode = readRequest(sockfd, &filename);

    if (returnCode == 400) {
        send400(sockfd);
    } else if (returnCode == 404) {
        send404(sockfd);
    } else if (returnCode == 200) {
        send200(sockfd, filename);
    }

    return 0;
}

// Read the request and determine the status
int readRequest(int socketFD, std::string *filename) {
    char buffer[MAX_BUFFER_SIZE];
    int bytesRead = read(socketFD, buffer, MAX_BUFFER_SIZE);
    
    if (bytesRead <= 0) {
        log_error("Failed to read from socket.");
        return 400;
    }

    std::string request(buffer, bytesRead);
    log_debug("Request: " + request);

    // Regex to match GET requests for fileX.html or imageX.jpg
    std::regex request_regex("GET\\s+/(file\\d\\.html|image\\d\\.jpg)\\s+HTTP/1\\.0\\r\\n");
    std::smatch match;

    if (std::regex_search(request, match, request_regex)) {
        *filename = match[1];
        log_info("Requested file: " + *filename);
        struct stat file_stat;
        if (stat(filename->c_str(), &file_stat) == 0) {
            return 200; // File exists
        } else {
            return 404; // File not found
        }
    }

    return 400; // Bad request
}

// Send a single line to the client with CRLF terminators
void sendLine(int socketFD, const std::string &stringToSend) {
    std::string line = stringToSend + "\r\n";
    write(socketFD, line.c_str(), line.size());
}

// Send 404 Not Found response
void send404(int socketFD) {
    log_info("Sending 404 Not Found.");
    sendLine(socketFD, "HTTP/1.0 404 Not Found");
    sendLine(socketFD, "Content-Type: text/html");
    sendLine(socketFD, "");
    sendLine(socketFD, "<html><body><h1>404 Not Found</h1></body></html>");
}

// Send 400 Bad Request response
void send400(int socketFD) {
    log_info("Sending 400 Bad Request.");
    sendLine(socketFD, "HTTP/1.0 400 Bad Request");
    sendLine(socketFD, "");
}

// Send 200 OK and the file content
void send200(int socketFD, const std::string &filename) {
    log_info("Sending 200 OK with file: " + filename);

    // Get file size
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) != 0) {
        send404(socketFD);
        return;
    }

    // Determine the content type
    std::string contentType = getContentType(filename);
    sendLine(socketFD, "HTTP/1.0 200 OK");
    sendLine(socketFD, "Content-Type: " + contentType);
    sendLine(socketFD, "Content-Length: " + std::to_string(file_stat.st_size));
    sendLine(socketFD, "");

    // Send the file content
    std::ifstream file(filename, std::ios::binary);
    char file_buffer[MAX_BUFFER_SIZE];
    while (file.read(file_buffer, sizeof(file_buffer)) || file.gcount() > 0) {
        write(socketFD, file_buffer, file.gcount());
    }
}

// Determine the content type based on the file extension
std::string getContentType(const std::string &filename) {
    if (filename.find(".html") != std::string::npos) {
        return "text/html";
    } else if (filename.find(".jpg") != std::string::npos) {
        return "image/jpeg";
    }
    return "application/octet-stream";
}
