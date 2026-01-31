#pragma once

#include <functional>
#include <string>
#include <thread>

namespace lab5 {

class HttpServer {
public:
 
    bool start(int port, std::function<std::pair<std::string, std::string>(const std::string&)> handler, std::string& err);
    void stop();

private:
    void run(int port);

    std::function<std::pair<std::string, std::string>(const std::string&)> handler_;
    bool running_ = false;
    std::thread thread_;
#ifdef _WIN32
    long long listen_fd_ = -1; 
#else
    int listen_fd_ = -1;
#endif
};

}  // namespace lab5
