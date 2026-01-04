# Package Manager Detection and Integration Module
# This module provides automated package manager detection and integration for Bolt C++

cmake_minimum_required(VERSION 3.15)

# Package manager detection function
function(detect_package_manager)
    set(BOLT_PACKAGE_MANAGER "none" PARENT_SCOPE)
    set(BOLT_VCPKG_AVAILABLE FALSE PARENT_SCOPE)
    set(BOLT_CONAN_AVAILABLE FALSE PARENT_SCOPE)
    
    # Check for vcpkg
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        set(BOLT_PACKAGE_MANAGER "vcpkg" PARENT_SCOPE)
        set(BOLT_VCPKG_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "Package manager: vcpkg (toolchain detected)")
        return()
    endif()
    
    # Check for vcpkg environment
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(BOLT_PACKAGE_MANAGER "vcpkg" PARENT_SCOPE)
        set(BOLT_VCPKG_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "Package manager: vcpkg (VCPKG_ROOT environment variable)")
        return()
    endif()
    
    # Check for vcpkg.json manifest
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
        set(BOLT_VCPKG_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "vcpkg manifest found but not configured as toolchain")
    endif()
    
    # Check for Conan
    find_program(CONAN_COMMAND conan)
    if(CONAN_COMMAND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt")
        set(BOLT_CONAN_AVAILABLE TRUE PARENT_SCOPE)
        if(NOT BOLT_PACKAGE_MANAGER STREQUAL "vcpkg")
            set(BOLT_PACKAGE_MANAGER "conan" PARENT_SCOPE)
            message(STATUS "Package manager: conan")
        else()
            message(STATUS "Conan available as alternative package manager")
        endif()
    endif()
    
    if(BOLT_PACKAGE_MANAGER STREQUAL "none")
        message(STATUS "Package manager: system dependencies only")
    endif()
endfunction()

# Enhanced dependency finding with fallback
function(bolt_find_package package_name)
    # Parse arguments
    set(options REQUIRED QUIET)
    set(oneValueArgs VERSION COMPONENT)
    set(multiValueArgs COMPONENTS)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Build find_package arguments
    set(find_args "${package_name}")
    if(ARGS_VERSION)
        list(APPEND find_args "${ARGS_VERSION}")
    endif()
    if(ARGS_COMPONENTS)
        list(APPEND find_args "COMPONENTS" ${ARGS_COMPONENTS})
    elseif(ARGS_COMPONENT)
        list(APPEND find_args "COMPONENTS" "${ARGS_COMPONENT}")
    endif()
    if(ARGS_QUIET)
        list(APPEND find_args "QUIET")
    endif()
    
    # Try CONFIG first (for vcpkg)
    find_package(${find_args} CONFIG QUIET)
    
    # Try MODULE mode if CONFIG failed and not quiet
    string(TOUPPER "${package_name}" package_name_upper)
    if(NOT ${package_name}_FOUND AND NOT ${package_name_upper}_FOUND AND NOT ARGS_QUIET)
        if(ARGS_REQUIRED)
            find_package(${find_args} REQUIRED)
        else()
            find_package(${find_args})
        endif()
    endif()
    
    # Set status variables in parent scope
    if(${package_name}_FOUND OR ${package_name_upper}_FOUND)
        set(BOLT_${package_name_upper}_FOUND TRUE PARENT_SCOPE)
        message(STATUS "Found ${package_name}")
    else()
        set(BOLT_${package_name_upper}_FOUND FALSE PARENT_SCOPE)
        if(ARGS_REQUIRED)
            message(FATAL_ERROR "Required package ${package_name} not found")
        else()
            message(STATUS "Package ${package_name} not found - related features will be disabled")
        endif()
    endif()
endfunction()

# Install missing system dependencies
function(install_system_dependencies)
    if(NOT UNIX)
        return() # Only support Unix-like systems for now
    endif()
    
    message(STATUS "Checking for required system dependencies...")
    
    # Check for X11 dependencies (Linux only)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11 QUIET)
        if(NOT X11_FOUND)
            message(WARNING "X11 development libraries not found. GUI features may be disabled.")
            message(STATUS "Install with: sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev")
        endif()
        
        # Check for OpenGL
        find_package(OpenGL QUIET)
        if(NOT OPENGL_FOUND)
            message(WARNING "OpenGL development libraries not found.")
            message(STATUS "Install with: sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev")
        endif()
    endif()
endfunction()

# Generate package manager status report
function(generate_package_status_report)
    message(STATUS "=== Package Manager Integration Status ===")
    message(STATUS "Primary package manager: ${BOLT_PACKAGE_MANAGER}")
    message(STATUS "vcpkg available: ${BOLT_VCPKG_AVAILABLE}")
    message(STATUS "Conan available: ${BOLT_CONAN_AVAILABLE}")
    
    # List found packages
    set(packages_to_check CURL JSONCPP GLFW3 IMGUI OPENGL)
    foreach(pkg IN LISTS packages_to_check)
        if(DEFINED BOLT_${pkg}_FOUND)
            if(BOLT_${pkg}_FOUND)
                message(STATUS "${pkg}: ✓ Found")
            else()
                message(STATUS "${pkg}: ✗ Not found")
            endif()
        endif()
    endforeach()
    message(STATUS "=========================================")
endfunction()

# Auto-configure package managers based on detection
function(configure_package_managers)
    detect_package_manager()
    install_system_dependencies()
    
    # Enable better error messages for missing packages
    set(CMAKE_FIND_PACKAGE_WARN_NO_MODULE ON PARENT_SCOPE)
    
    # Use modern CMake targets when available
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON PARENT_SCOPE)
endfunction()

# Package-specific integration helpers
function(bolt_link_package target package_name)
    string(TOUPPER "${package_name}" package_upper)
    
    if(NOT BOLT_${package_upper}_FOUND)
        return()
    endif()
    
    # Try different target name patterns
    set(possible_targets 
        "${package_name}::${package_name}"
        "${package_name}::${package_upper}"
        "${package_upper}::${package_upper}"
        "${package_upper}::${package_name}"
    )
    
    foreach(target_name IN LISTS possible_targets)
        if(TARGET ${target_name})
            target_link_libraries(${target} PRIVATE ${target_name})
            message(STATUS "Linked ${target} -> ${target_name}")
            return()
        endif()
    endforeach()
    
    # Fallback to variables
    if(DEFINED ${package_upper}_LIBRARIES AND DEFINED ${package_upper}_INCLUDE_DIRS)
        target_link_libraries(${target} PRIVATE ${${package_upper}_LIBRARIES})
        target_include_directories(${target} PRIVATE ${${package_upper}_INCLUDE_DIRS})
        message(STATUS "Linked ${target} -> ${package_name} (variables)")
    endif()
endfunction()

# Export functions for use in parent scope
set(BOLT_PACKAGE_MANAGER_FUNCTIONS_LOADED TRUE)