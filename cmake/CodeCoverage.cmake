# Code Coverage Configuration for Bolt C++
#
# This file provides CMake targets for generating code coverage reports
# using gcov and lcov. 
#
# Usage:
#   1. Configure with coverage enabled:
#      cmake -DENABLE_COVERAGE=ON ..
#   
#   2. Build the project:
#      make
#   
#   3. Run tests:
#      make test
#   
#   4. Generate coverage report:
#      make coverage
#
#   5. View HTML report:
#      Open build/coverage/index.html in a browser

# Coverage option (off by default)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage reporting enabled")
    
    # Check for required tools
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(NOT LCOV_PATH)
        message(WARNING "lcov not found! Coverage reports will not be available. Install with: sudo apt-get install lcov")
    endif()
    
    if(NOT GENHTML_PATH)
        message(WARNING "genhtml not found! HTML coverage reports will not be available.")
    endif()
    
    # Add coverage compiler flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        
        message(STATUS "Coverage compiler flags added: ${COVERAGE_COMPILER_FLAGS}")
    else()
        message(WARNING "Code coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
    
    if(LCOV_PATH AND GENHTML_PATH)
        # Setup coverage target
        add_custom_target(coverage
            COMMENT "Generating code coverage report..."
            
            # Cleanup old coverage data
            COMMAND ${LCOV_PATH} --directory . --zerocounters
            
            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            
            # Capture coverage data
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            
            # Remove unwanted coverage (system headers, test files)
            COMMAND ${LCOV_PATH} --remove coverage.info 
                '/usr/*' 
                '*/test/*' 
                '*/ggml/*'
                '*/vcpkg_installed/*'
                --output-file coverage.info.cleaned
            
            # Generate HTML report
            COMMAND ${GENHTML_PATH} coverage.info.cleaned --output-directory coverage
            
            # Summary
            COMMAND ${LCOV_PATH} --summary coverage.info.cleaned
            
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        
        # Coverage depends on tests being built
        add_custom_target(coverage-prepare
            COMMENT "Building tests for coverage..."
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bolt_unit_tests
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bolt_integration_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_dependencies(coverage coverage-prepare)
        
        # Add a coverage-clean target to remove old coverage data
        add_custom_target(coverage-clean
            COMMAND ${LCOV_PATH} --directory . --zerocounters
            COMMAND ${CMAKE_COMMAND} -E remove_directory coverage
            COMMAND ${CMAKE_COMMAND} -E remove coverage.info coverage.info.cleaned
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Cleaning coverage data..."
        )
        
        message(STATUS "Coverage targets available:")
        message(STATUS "  make coverage         - Generate coverage report")
        message(STATUS "  make coverage-clean   - Clean coverage data")
        message(STATUS "  Report will be in: ${CMAKE_BINARY_DIR}/coverage/index.html")
    else()
        message(WARNING "lcov and/or genhtml not found. Coverage target not available.")
    endif()
else()
    message(STATUS "Code coverage reporting disabled (use -DENABLE_COVERAGE=ON to enable)")
endif()

# Function to add coverage exclude pattern
function(add_coverage_exclude)
    # This can be extended in the future to support custom exclude patterns
endfunction()
