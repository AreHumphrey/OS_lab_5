#include "simulator.h"

#include <random>

#include "common.h"

namespace lab5 {

void Simulator::start() {
    running_ = true;
    thread_ = std::thread([this]() { run(); });
}

void Simulator::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

bool Simulator::pop(Sample& out) {
    std::unique_lock<std::mutex> lk(mu_);
    cv_.wait(lk, [&] { return !queue_.empty() || !running_; });
    if (queue_.empty()) return false;
    out = queue_.front();
    queue_.pop();
    return true;
}

void Simulator::run() {
    std::mt19937 rng(static_cast<unsigned>(now_ms()));
    std::uniform_real_distribution<double> dist(-10.0, 35.0);
    while (running_) {
        Sample s{Clock::now(), dist(rng)};
        {
            std::lock_guard<std::mutex> lk(mu_);
            queue_.push(s);
        }
        cv_.notify_one();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    cv_.notify_all();
}

}  // namespace lab5
