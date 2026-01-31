#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "sample.h"

namespace lab5 {

class Simulator {
public:
    void start();
    void stop();
    bool pop(Sample& out);

private:
    void run();

    std::atomic<bool> running_{false};
    std::thread thread_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::queue<Sample> queue_;
};

}  // namespace lab5
