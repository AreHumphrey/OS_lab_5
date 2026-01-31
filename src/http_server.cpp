#include "http_server.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace lab5 {

void HttpServer::run(int port) {
    #ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return;
    }
    #endif

    #ifdef _WIN32
    listen_fd_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    #else
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    #endif
    
    if (listen_fd_ < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

    int opt = 1;
    #ifdef _WIN32
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    #else
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listen_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        #ifdef _WIN32
        closesocket(listen_fd_);
        #else
        close(listen_fd_);
        #endif
        return;
    }

    if (listen(listen_fd_, 5) < 0) {
        std::cerr << "Listen failed" << std::endl;
        #ifdef _WIN32
        closesocket(listen_fd_);
        #else
        close(listen_fd_);
        #endif
        return;
    }

    std::cout << "HTTP server running on port " << port << std::endl;
    
    while (running_) {
        #ifdef _WIN32
        SOCKET client_fd = accept(listen_fd_, NULL, NULL);
        #else
        int client_fd = accept(listen_fd_, NULL, NULL);
        #endif
        
        if (client_fd < 0) {
            continue;
        }

        std::thread([this, client_fd]() {
            char buffer[2048];
            ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::string request(buffer, bytes_received);
                std::string response;
                std::string content_type = "text/html";
                
                // Извлекаем путь из запроса
                std::string path;
                size_t path_start = request.find("GET ") + 4;
                size_t path_end = request.find(" HTTP/1.1");
                if (path_start != std::string::npos && path_end != std::string::npos) {
                    path = request.substr(path_start, path_end - path_start);
                    if (path.empty() || path == "/") {
                        path = "/index.html";
                    }
                }
                
                // Обработка API
                if (path.find("/api/current") == 0 || path.find("/api/stats") == 0) {
                    auto [body, ct] = handler_(path);
                    response = "HTTP/1.1 200 OK\r\nContent-Type: " + ct + "\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
                } else {
                    // Обработка статических файлов
                    std::string filepath = "web/public" + path;
                    
                    // Определяем тип контента
                    if (filepath.find(".css") != std::string::npos) content_type = "text/css";
                    else if (filepath.find(".js") != std::string::npos) content_type = "application/javascript";
                    else if (filepath.find(".json") != std::string::npos) content_type = "application/json";
                    
                    std::ifstream file(filepath, std::ios::binary);
                    if (file.is_open()) {
                        std::ostringstream oss;
                        oss << file.rdbuf();
                        std::string content = oss.str();
                        file.close();
                        
                        response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nConnection: close\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
                    } else {
                        std::string error = "<h1>404 Not Found</h1><p>File: " + filepath + "</p>";
                        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: " + std::to_string(error.size()) + "\r\n\r\n" + error;
                    }
                }
                
                send(client_fd, response.c_str(), response.size(), 0);
            }
            
            #ifdef _WIN32
            closesocket(client_fd);
            #else
            close(client_fd);
            #endif
        }).detach();
    }
    
    #ifdef _WIN32
    closesocket(listen_fd_);
    WSACleanup();
    #else
    close(listen_fd_);
    #endif
}

bool HttpServer::start(int port, std::function<std::pair<std::string, std::string>(const std::string&)> handler, std::string& err) {
    handler_ = handler;
    running_ = true;
    
    try {
        thread_ = std::thread([this, port]() { run(port); });
        return true;
    } catch (const std::exception& e) {
        err = "Failed to start server: " + std::string(e.what());
        return false;
    }
}

void HttpServer::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

}  // namespace lab5
