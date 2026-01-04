# Sanitizers.cmake - Memory leak detection and sanitizer support
# This module provides options to enable various sanitizers for detecting
# memory leaks, undefined behavior, and other runtime issues.

option(ENABLE_SANITIZER_ADDRESS "Enable AddressSanitizer" OFF)
option(ENABLE_SANITIZER_LEAK "Enable LeakSanitizer" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_SANITIZER_THREAD "Enable ThreadSanitizer" OFF)
option(ENABLE_SANITIZER_MEMORY "Enable MemorySanitizer" OFF)
option(ENABLE_VALGRIND "Enable Valgrind support" OFF)

# Function to add sanitizer flags to a target
function(add_sanitizer_flags target_name)
    set(SANITIZER_FLAGS "")
    set(SANITIZER_LINK_FLAGS "")
    
    # AddressSanitizer (ASan) - Detects memory errors
    if(ENABLE_SANITIZER_ADDRESS)
        message(STATUS "Building with AddressSanitizer enabled")
        list(APPEND SANITIZER_FLAGS 
            -fsanitize=address
            -fno-omit-frame-pointer
            -g
        )
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=address)
        
        # ASan options for better leak detection
        target_compile_definitions(${target_name} PRIVATE 
            ASAN_ENABLED=1
        )
    endif()
    
    # LeakSanitizer (LSan) - Detects memory leaks
    if(ENABLE_SANITIZER_LEAK)
        message(STATUS "Building with LeakSanitizer enabled")
        list(APPEND SANITIZER_FLAGS 
            -fsanitize=leak
            -fno-omit-frame-pointer
            -g
        )
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=leak)
        
        target_compile_definitions(${target_name} PRIVATE 
            LSAN_ENABLED=1
        )
    endif()
    
    # UndefinedBehaviorSanitizer (UBSan) - Detects undefined behavior
    if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
        message(STATUS "Building with UndefinedBehaviorSanitizer enabled")
        list(APPEND SANITIZER_FLAGS 
            -fsanitize=undefined
            -fno-omit-frame-pointer
            -g
        )
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=undefined)
        
        target_compile_definitions(${target_name} PRIVATE 
            UBSAN_ENABLED=1
        )
    endif()
    
    # ThreadSanitizer (TSan) - Detects data races
    if(ENABLE_SANITIZER_THREAD)
        message(STATUS "Building with ThreadSanitizer enabled")
        list(APPEND SANITIZER_FLAGS 
            -fsanitize=thread
            -fno-omit-frame-pointer
            -g
        )
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=thread)
        
        target_compile_definitions(${target_name} PRIVATE 
            TSAN_ENABLED=1
        )
        
        # Note: ThreadSanitizer is incompatible with Address/Leak sanitizers
        if(ENABLE_SANITIZER_ADDRESS OR ENABLE_SANITIZER_LEAK)
            message(WARNING "ThreadSanitizer is incompatible with AddressSanitizer and LeakSanitizer")
        endif()
    endif()
    
    # MemorySanitizer (MSan) - Detects uninitialized memory reads (Clang only)
    if(ENABLE_SANITIZER_MEMORY)
        message(STATUS "Building with MemorySanitizer enabled")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            list(APPEND SANITIZER_FLAGS 
                -fsanitize=memory
                -fno-omit-frame-pointer
                -g
            )
            list(APPEND SANITIZER_LINK_FLAGS -fsanitize=memory)
            
            target_compile_definitions(${target_name} PRIVATE 
                MSAN_ENABLED=1
            )
        else()
            message(WARNING "MemorySanitizer requires Clang compiler")
        endif()
    endif()
    
    # Apply the collected flags
    if(SANITIZER_FLAGS)
        target_compile_options(${target_name} PRIVATE ${SANITIZER_FLAGS})
        target_link_options(${target_name} PRIVATE ${SANITIZER_LINK_FLAGS})
    endif()
endfunction()

# Function to add Valgrind support
function(add_valgrind_test test_name test_target)
    if(ENABLE_VALGRIND)
        find_program(VALGRIND_EXECUTABLE valgrind)
        
        if(VALGRIND_EXECUTABLE)
            message(STATUS "Valgrind found: ${VALGRIND_EXECUTABLE}")
            
            # Add a test that runs with Valgrind
            add_test(
                NAME ${test_name}_valgrind
                COMMAND ${VALGRIND_EXECUTABLE}
                    --leak-check=full
                    --show-leak-kinds=all
                    --track-origins=yes
                    --verbose
                    --error-exitcode=1
                    --suppressions=${CMAKE_SOURCE_DIR}/cmake/valgrind.supp
                    $<TARGET_FILE:${test_target}>
            )
            
            # Set environment for better output
            set_tests_properties(${test_name}_valgrind PROPERTIES
                ENVIRONMENT "VALGRIND_ENABLED=1"
                TIMEOUT 600
            )
        else()
            message(WARNING "Valgrind not found. Install valgrind to enable memory leak detection with Valgrind.")
        endif()
    endif()
endfunction()

# Preset configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Recommend enabling leak detection in debug builds
    message(STATUS "Debug build detected. Consider enabling sanitizers with -DENABLE_SANITIZER_ADDRESS=ON")
endif()

# Check for sanitizer compatibility
if(ENABLE_SANITIZER_ADDRESS AND ENABLE_SANITIZER_THREAD)
    message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be used together")
endif()

if(ENABLE_SANITIZER_LEAK AND ENABLE_SANITIZER_THREAD)
    message(FATAL_ERROR "LeakSanitizer and ThreadSanitizer cannot be used together")
endif()

if(ENABLE_SANITIZER_MEMORY AND (ENABLE_SANITIZER_ADDRESS OR ENABLE_SANITIZER_THREAD))
    message(FATAL_ERROR "MemorySanitizer cannot be used with AddressSanitizer or ThreadSanitizer")
endif()

# Export sanitizer status
set(SANITIZERS_ENABLED FALSE)
if(ENABLE_SANITIZER_ADDRESS OR ENABLE_SANITIZER_LEAK OR 
   ENABLE_SANITIZER_UNDEFINED_BEHAVIOR OR ENABLE_SANITIZER_THREAD OR
   ENABLE_SANITIZER_MEMORY)
    set(SANITIZERS_ENABLED TRUE)
endif()

# Print summary
if(SANITIZERS_ENABLED OR ENABLE_VALGRIND)
    message(STATUS "=== Sanitizers Configuration ===")
    
    # Check each sanitizer individually
    if(ENABLE_SANITIZER_ADDRESS)
        message(STATUS "  AddressSanitizer: ENABLED")
    endif()
    if(ENABLE_SANITIZER_LEAK)
        message(STATUS "  LeakSanitizer: ENABLED")
    endif()
    if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
        message(STATUS "  UndefinedBehaviorSanitizer: ENABLED")
    endif()
    if(ENABLE_SANITIZER_THREAD)
        message(STATUS "  ThreadSanitizer: ENABLED")
    endif()
    if(ENABLE_SANITIZER_MEMORY)
        message(STATUS "  MemorySanitizer: ENABLED")
    endif()
    if(ENABLE_VALGRIND)
        message(STATUS "  Valgrind: ENABLED")
    endif()
    
    message(STATUS "================================")
endif()
