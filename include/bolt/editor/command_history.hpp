
#ifndef COMMAND_HISTORY_HPP
#define COMMAND_HISTORY_HPP

#include <deque>
#include <string>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

class CommandHistory {
private:
    ThreadSafe<std::deque<std::string>> history_;
    ThreadSafe<size_t> maxSize_{100};
    ThreadSafe<size_t> currentIndex_{0};

public:
    static CommandHistory& getInstance() {
        static CommandHistory instance;
        return instance;
    }

    void addCommand(const std::string& command) {
        history_.write([&](auto& hist) {
            if (hist.size() >= maxSize_.read()) {
                hist.pop_front();
            }
            hist.push_back(command);
            currentIndex_.write([&](auto& idx) {
                idx = hist.size();
                return idx;
            });
        });
    }

    std::string getPreviousCommand() {
        return history_.read([&](const auto& hist) {
            if (hist.empty()) return std::string();
            
            size_t idx = currentIndex_.write([&](auto& current) {
                if (current > 0) current--;
                return current;
            });
            
            return idx < hist.size() ? hist[idx] : std::string();
        });
    }

    std::string getNextCommand() {
        return history_.read([&](const auto& hist) {
            if (hist.empty()) return std::string();
            
            size_t idx = currentIndex_.write([&](auto& current) {
                if (current < hist.size()) current++;
                return current;
            });
            
            return idx < hist.size() ? hist[idx] : std::string();
        });
    }
};

} // namespace bolt

#endif
