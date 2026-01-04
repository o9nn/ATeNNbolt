
#include "bolt/bolt.hpp"
#include <iostream>

int main() {
    try {
        auto& app = bolt::BoltApp::getInstance();
        app.initialize();
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
