#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex>

#define MAX_BUFFER_SIZE 1024

int LOG_LEVEL = 0; // Default log level

void log_info(const std::string &msg) {
    if (LOG_LEVEL >= 3) {
        std::cerr << "INFO: " << msg << std::endl;
    }
}

void sendLine(int sockfd, const std::string& str) {
    std::string line = str + "\r\n";
    send(sockfd, line.c_str(), line.length(), 0);
}

void send404(int sockfd) {
    sendLine(sockfd, "HTTP/1.0 404 Not Found");
    sendLine(sockfd, "Content-Type: text/html");
    sendLine(sockfd, "");
    sendLine(sockfd, "<html><body><h1>404 Not Found</h1></body></html>");
}

void send400(int sockfd) {
    sendLine(sockfd, "HTTP/1.0 400 Bad Request");
    sendLine(sockfd, "");
}

void sendFile(int sockfd, const std::string& filename) {
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) < 0) {
        send404(sockfd);
        return;
    }

    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(sockfd);
        return;
    }

    std::string content_type;
    if (filename.find(".html") != std::string::npos) {
        content_type = "text/html";
    } else if (filename.find(".jpg") != std::string::npos) {
        content_type = "image/jpeg";
    } else {
        send404(sockfd);
        return;
    }

    sendLine(sockfd, "HTTP/1.0 200 OK");
    sendLine(sockfd, "Content-Type: " + content_type);
    sendLine(sockfd, "Content-Length: " + std::to_string(file_stat.st_size));
    sendLine(sockfd, "");

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        send(sockfd, buffer, bytes_read, 0);
    }
    close(fd);
}

int readRequest(int sockfd, std::string *filename) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return 400;
    }

    buffer[bytes_read] = '\0';
    std::string request(buffer);

    std::regex get_regex("^GET\\s+/(file[0-9]\\.html|image[0-9]\\.jpg)\\s+HTTP/1\\.0\r\n");
    std::smatch match;

    if (std::regex_search(request, match, get_regex)) {
        *filename = match[1];
        return 200;
    } else {
        return 400;
    }
}

void processConnection(int sockfd) {
    std::string filename;
    int status = readRequest(sockfd, &filename);

    if (status == 400) {
        send400(sockfd);
    } else if (status == 200) {
        sendFile(sockfd, filename);
    }
}

int main(int argc, char *argv[]) {
    int port = 8080;  // You can choose any port > 1023
    if (argc == 3 && std::string(argv[1]) == "-d") {
        LOG_LEVEL = std::stoi(argv[2]);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    listen(sockfd, 10);
    log_info("Server listening on port " + std::to_string(port));

    while (true) {
        int client_sock = accept(sockfd, nullptr, nullptr);
        if (client_sock >= 0) {
            processConnection(client_sock);
            close(client_sock);
        }
    }

    close(sockfd);
    return 0;
}
