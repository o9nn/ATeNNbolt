
#ifndef LINE_NUMBERS_HPP
#define LINE_NUMBERS_HPP

#include <string>
#include <vector>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

class LineNumberManager {
private:
    ThreadSafe<bool> showLineNumbers_{true};
    ThreadSafe<std::string> lineNumberFormat_{"%d"};
    ThreadSafe<size_t> gutterWidth_{4};

public:
    static LineNumberManager& getInstance() {
        static LineNumberManager instance;
        return instance;
    }

    void toggleLineNumbers() {
        showLineNumbers_.write([](bool& show) {
            show = !show;
        });
    }

    bool areLineNumbersVisible() const {
        return showLineNumbers_.read([](const bool& show) { return show; });
    }

    void setGutterWidth(size_t width) {
        gutterWidth_.write([width](size_t& w) {
            w = width;
        });
    }

    size_t getGutterWidth() const {
        return gutterWidth_.read([](const size_t& w) { return w; });
    }

    std::string formatLineNumber(size_t lineNumber) const {
        char buffer[32];
        std::string format = lineNumberFormat_.read([](const auto& f) { return f; });
        snprintf(buffer, sizeof(buffer), format.c_str(), lineNumber);
        return std::string(buffer);
    }

    std::vector<std::string> generateLineNumbers(size_t totalLines) const {
        std::vector<std::string> numbers;
        numbers.reserve(totalLines);
        for (size_t i = 1; i <= totalLines; ++i) {
            numbers.push_back(formatLineNumber(i));
        }
        return numbers;
    }
};

} // namespace bolt

#endif
