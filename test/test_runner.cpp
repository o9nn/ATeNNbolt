#include "bolt/test_framework.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    using namespace bolt::test;
    
    std::string suite_name = "";
    bool list_tests = false;
    bool verbose = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--list" || arg == "-l") {
            list_tests = true;
        } else if (arg == "--quiet" || arg == "-q") {
            verbose = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Bolt C++ Unit Test Runner\n" << std::endl;
            std::cout << "Usage: " << argv[0] << " [options] [suite_name]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --help    Show this help message" << std::endl;
            std::cout << "  -l, --list    List all available tests" << std::endl;
            std::cout << "  -q, --quiet   Run tests in quiet mode" << std::endl;
            std::cout << std::endl;
            std::cout << "Arguments:" << std::endl;
            std::cout << "  suite_name    Run only tests in the specified suite" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  " << argv[0] << "              # Run all tests" << std::endl;
            std::cout << "  " << argv[0] << " Chat         # Run only Chat tests" << std::endl;
            std::cout << "  " << argv[0] << " --list       # List all tests" << std::endl;
            return 0;
        } else if (arg[0] != '-') {
            suite_name = arg;
        }
    }
    
    auto& testSuite = TestSuite::getInstance();
    
    if (list_tests) {
        testSuite.listTests();
        return 0;
    }
    
    int failed_tests = 0;
    
    if (suite_name.empty()) {
        if (verbose) {
            std::cout << "Bolt C++ Unit Test Runner" << std::endl;
            std::cout << "=========================" << std::endl;
        }
        failed_tests = testSuite.runAllTests(verbose);
    } else {
        if (verbose) {
            std::cout << "Bolt C++ Unit Test Runner - Suite: " << suite_name << std::endl;
            std::cout << "===========================================" << std::endl;
        }
        failed_tests = testSuite.runTestsInSuite(suite_name, verbose);
    }
    
    return failed_tests;
}