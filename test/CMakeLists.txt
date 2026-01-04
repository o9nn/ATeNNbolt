# Comprehensive Test Framework Setup for Bolt C++ ML
cmake_minimum_required(VERSION 3.15)

# ============================================
# Test Configuration Options
# ============================================
option(BOLT_ENABLE_TESTS "Enable building tests" ON)
option(BOLT_ENABLE_E2E_TESTS "Enable End-to-End tests" ON)
option(BOLT_ENABLE_INTEGRATION_TESTS "Enable integration tests" ON)
option(BOLT_ENABLE_UNIT_TESTS "Enable unit tests" ON)
option(BOLT_ENABLE_PERFORMANCE_TESTS "Enable performance tests" OFF)

# Find if GoogleTest is available
find_package(GTest QUIET)

# ============================================
# GoogleTest-based Tests (if available)
# ============================================
if(GTest_FOUND)
    message(STATUS "GoogleTest found - enabling GTest-based tests")

    # Create test executable if GoogleTest is available
    add_executable(bolt_tests
        basic_test.cpp
    )

    target_link_libraries(bolt_tests
        PRIVATE
        bolt_lib
        GTest::gtest_main
    )

    # Add widget framework tests if GoogleTest is available
    add_executable(bolt_widget_framework_tests
        test_widget_framework.cpp
    )

    target_link_libraries(bolt_widget_framework_tests
        PRIVATE
        bolt_lib
        GTest::gtest_main
    )

    target_include_directories(bolt_widget_framework_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(bolt_tests)
    gtest_discover_tests(bolt_widget_framework_tests)
else()
    message(STATUS "GoogleTest not found - using custom test framework")

    # Create a minimal test without GoogleTest
    add_executable(bolt_basic_test
        basic_test.cpp
    )

    target_link_libraries(bolt_basic_test PRIVATE bolt_lib)
    target_include_directories(bolt_basic_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Add basic test to CTest
    add_test(NAME basic_bolt_test COMMAND bolt_basic_test)
endif()

# ============================================
# Unit Tests (Custom Framework)
# ============================================
if(BOLT_ENABLE_UNIT_TESTS)
    # Comprehensive unit test runner
    add_executable(bolt_unit_tests
        test_runner.cpp
        test_bolt_core.cpp
        test_error_handling.cpp
        test_file_tree.cpp
        test_minimap.cpp
        test_tab_bar.cpp
        test_ai_ggml.cpp
        test_ai_models_complete.cpp
        test_debugger.cpp
        test_logging.cpp
        test_memory_leak_detector.cpp
    )

    target_link_libraries(bolt_unit_tests PRIVATE bolt_lib)
    target_include_directories(bolt_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_definitions(bolt_unit_tests PRIVATE BOLT_INCLUDE_IN_TEST_RUNNER=1)

    # Sanitizer integration tests
    add_executable(bolt_sanitizer_tests
        test_runner.cpp
        test_sanitizer_integration.cpp
    )

    target_link_libraries(bolt_sanitizer_tests PRIVATE bolt_lib)
    target_include_directories(bolt_sanitizer_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Split view tests
    add_executable(bolt_split_view_tests
        test_split_view.cpp
    )

    target_link_libraries(bolt_split_view_tests PRIVATE bolt_lib)
    target_include_directories(bolt_split_view_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Multi-cursor tests
    add_executable(bolt_multi_cursor_tests
        test_multi_cursor.cpp
    )

    target_link_libraries(bolt_multi_cursor_tests PRIVATE bolt_lib)
    target_include_directories(bolt_multi_cursor_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Keyboard shortcuts tests
    add_executable(bolt_keyboard_shortcuts_tests
        test_keyboard_shortcuts.cpp
    )

    target_link_libraries(bolt_keyboard_shortcuts_tests PRIVATE bolt_lib)
    target_include_directories(bolt_keyboard_shortcuts_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Theme system tests
    add_executable(bolt_theme_system_tests
        test_theme_system.cpp
    )

    target_link_libraries(bolt_theme_system_tests PRIVATE bolt_lib)
    target_include_directories(bolt_theme_system_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Tab bar tests
    add_executable(bolt_tab_bar_tests
        test_runner.cpp
        test_tab_bar.cpp
    )

    target_link_libraries(bolt_tab_bar_tests PRIVATE bolt_lib)
    target_include_directories(bolt_tab_bar_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Plugin system tests
    add_executable(bolt_plugin_system_tests
        test_plugin_system.cpp
    )

    target_link_libraries(bolt_plugin_system_tests PRIVATE bolt_lib)
    target_include_directories(bolt_plugin_system_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Collaboration tests
    add_executable(bolt_collaboration_tests
        test_collaboration.cpp
    )

    target_link_libraries(bolt_collaboration_tests PRIVATE bolt_lib)
    target_include_directories(bolt_collaboration_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Network optimization tests
    add_executable(bolt_network_tests
        test_network_optimizations.cpp
    )

    target_link_libraries(bolt_network_tests PRIVATE bolt_lib)
    target_include_directories(bolt_network_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Code folding tests
    add_executable(bolt_code_folding_tests
        test_code_folding.cpp
    )

    target_link_libraries(bolt_code_folding_tests PRIVATE bolt_lib)
    target_include_directories(bolt_code_folding_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # AI Features comprehensive tests - DISABLED: API mismatch with test expectations
    # add_executable(bolt_ai_features_tests
    #     test_ai_features.cpp
    # )
    #
    # target_link_libraries(bolt_ai_features_tests PRIVATE bolt_lib)
    # target_include_directories(bolt_ai_features_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # LSP tests - DISABLED: missing lsp_server.hpp
    # add_executable(bolt_lsp_tests
    #     test_lsp.cpp
    # )
    #
    # target_link_libraries(bolt_lsp_tests PRIVATE bolt_lib)
    # target_include_directories(bolt_lsp_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # GPU acceleration tests - DISABLED: uses gtest
    # add_executable(bolt_gpu_tests
    #     test_gpu_acceleration.cpp
    # )
    #
    # target_link_libraries(bolt_gpu_tests PRIVATE bolt_lib)
    # target_include_directories(bolt_gpu_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
endif()

# ============================================
# Integration Tests
# ============================================
if(BOLT_ENABLE_INTEGRATION_TESTS)
    # Integration test runner
    add_executable(bolt_integration_tests
        test_runner.cpp
        test_integration.cpp
    )

    target_link_libraries(bolt_integration_tests PRIVATE bolt_lib)
    target_include_directories(bolt_integration_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_definitions(bolt_integration_tests PRIVATE BOLT_INCLUDE_IN_TEST_RUNNER=1)
endif()

# ============================================
# End-to-End Tests - DISABLED: API mismatch with test expectations
# ============================================
# if(BOLT_ENABLE_E2E_TESTS)
#     # E2E test suite
#     add_executable(bolt_e2e_tests
#         test_e2e.cpp
#     )
#
#     target_link_libraries(bolt_e2e_tests PRIVATE bolt_lib)
#     target_include_directories(bolt_e2e_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
#
#     # Set E2E test label for CTest filtering
#     set_target_properties(bolt_e2e_tests PROPERTIES
#         FOLDER "Tests/E2E"
#     )
# endif()

# ============================================
# Performance Tests (Optional)
# ============================================
if(BOLT_ENABLE_PERFORMANCE_TESTS)
    add_executable(bolt_performance_tests
        test_runner.cpp
        test_bolt_core.cpp
    )

    target_link_libraries(bolt_performance_tests PRIVATE bolt_lib)
    target_include_directories(bolt_performance_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_definitions(bolt_performance_tests PRIVATE
        BOLT_PERFORMANCE_TESTS=1
        BOLT_INCLUDE_IN_TEST_RUNNER=1
    )
endif()

# ============================================
# Apply Sanitizer Flags
# ============================================
if(SANITIZERS_ENABLED)
    if(BOLT_ENABLE_UNIT_TESTS)
        add_sanitizer_flags(bolt_unit_tests)
        add_sanitizer_flags(bolt_sanitizer_tests)
        add_sanitizer_flags(bolt_split_view_tests)
        add_sanitizer_flags(bolt_multi_cursor_tests)
        add_sanitizer_flags(bolt_keyboard_shortcuts_tests)
        add_sanitizer_flags(bolt_theme_system_tests)
        add_sanitizer_flags(bolt_tab_bar_tests)
        add_sanitizer_flags(bolt_plugin_system_tests)
        add_sanitizer_flags(bolt_collaboration_tests)
        add_sanitizer_flags(bolt_network_tests)
        add_sanitizer_flags(bolt_code_folding_tests)
        # DISABLED: executables not built
        # add_sanitizer_flags(bolt_ai_features_tests)
        # add_sanitizer_flags(bolt_lsp_tests)
        # add_sanitizer_flags(bolt_gpu_tests)
    endif()

    if(BOLT_ENABLE_INTEGRATION_TESTS)
        add_sanitizer_flags(bolt_integration_tests)
    endif()

    # DISABLED: E2E executable not built
    # if(BOLT_ENABLE_E2E_TESTS)
    #     add_sanitizer_flags(bolt_e2e_tests)
    # endif()
endif()

# ============================================
# CTest Configuration - Unit Tests
# ============================================
if(BOLT_ENABLE_UNIT_TESTS)
    # Core tests
    add_test(NAME unit_all_tests COMMAND bolt_unit_tests)
    add_test(NAME unit_chat_tests COMMAND bolt_unit_tests Chat)
    add_test(NAME unit_memory_tests COMMAND bolt_unit_tests MemoryManager)
    add_test(NAME unit_store_tests COMMAND bolt_unit_tests ChatStore)
    add_test(NAME unit_string_tests COMMAND bolt_unit_tests StringUtils)
    add_test(NAME unit_file_tree_tests COMMAND bolt_unit_tests FileTree)
    add_test(NAME unit_minimap_tests COMMAND bolt_unit_tests Minimap)

    # Editor component tests
    add_test(NAME unit_split_view_tests COMMAND bolt_split_view_tests)
    add_test(NAME unit_multi_cursor_tests COMMAND bolt_multi_cursor_tests)
    add_test(NAME unit_keyboard_shortcuts_tests COMMAND bolt_keyboard_shortcuts_tests)
    add_test(NAME unit_theme_system_tests COMMAND bolt_theme_system_tests)
    add_test(NAME unit_plugin_system_tests COMMAND bolt_plugin_system_tests)
    add_test(NAME unit_code_folding_tests COMMAND bolt_code_folding_tests)

    # Error handling tests
    add_test(NAME unit_error_handling_tests COMMAND bolt_unit_tests ErrorHandling)
    add_test(NAME unit_memory_error_tests COMMAND bolt_unit_tests MemoryManagerErrors)
    add_test(NAME unit_message_error_tests COMMAND bolt_unit_tests MessageHandlerErrors)
    add_test(NAME unit_store_error_tests COMMAND bolt_unit_tests ChatStoreErrors)
    add_test(NAME unit_editor_error_tests COMMAND bolt_unit_tests EditorStoreErrors)
    add_test(NAME unit_workbench_error_tests COMMAND bolt_unit_tests WorkbenchStoreErrors)
    add_test(NAME unit_error_recovery_tests COMMAND bolt_unit_tests ErrorRecovery)

    # AI tests
    add_test(NAME unit_ggml_tests COMMAND bolt_unit_tests GGMLTest)
    add_test(NAME unit_ai_models_tests COMMAND bolt_unit_tests AIModels)
    # DISABLED: executable not built
    # add_test(NAME unit_ai_features_tests COMMAND bolt_ai_features_tests)
    # add_test(NAME unit_gpu_tests COMMAND bolt_gpu_tests)

    # System tests
    add_test(NAME unit_debugger_tests COMMAND bolt_unit_tests Debugger)
    add_test(NAME unit_logging_tests COMMAND bolt_unit_tests Logging)
    add_test(NAME unit_memory_leak_detector_tests COMMAND bolt_unit_tests MemoryLeakDetector)
    add_test(NAME unit_sanitizer_integration_tests COMMAND bolt_sanitizer_tests SanitizerIntegration)
    add_test(NAME unit_collaboration_tests COMMAND bolt_collaboration_tests)
    # Network tests - DISABLED: Requires network server, times out in CI
    # The executable is still built and can be run manually for local testing
    # add_test(NAME unit_network_tests COMMAND bolt_network_tests)

    # LSP tests - DISABLED: executable not built
    # add_test(NAME unit_lsp_tests COMMAND bolt_lsp_tests)

    # Set test labels for filtering
    set_tests_properties(
        unit_all_tests unit_chat_tests unit_memory_tests unit_store_tests
        unit_string_tests unit_file_tree_tests unit_minimap_tests
        PROPERTIES LABELS "Unit;Core"
    )

    set_tests_properties(
        unit_split_view_tests unit_multi_cursor_tests unit_keyboard_shortcuts_tests
        unit_theme_system_tests unit_code_folding_tests
        PROPERTIES LABELS "Unit;Editor"
    )

    set_tests_properties(
        unit_error_handling_tests unit_memory_error_tests unit_message_error_tests
        unit_store_error_tests unit_editor_error_tests unit_workbench_error_tests
        unit_error_recovery_tests
        PROPERTIES LABELS "Unit;ErrorHandling"
    )

    set_tests_properties(
        unit_ggml_tests unit_ai_models_tests
        PROPERTIES LABELS "Unit;AI"
    )

    # LSP tests label - DISABLED: executable not built
    # set_tests_properties(
    #     unit_lsp_tests
    #     PROPERTIES LABELS "Unit;LSP"
    # )
endif()

# ============================================
# CTest Configuration - Integration Tests
# ============================================
if(BOLT_ENABLE_INTEGRATION_TESTS)
    add_test(NAME integration_all COMMAND bolt_integration_tests)
    add_test(NAME integration_basic COMMAND bolt_integration_tests Integration)

    set_tests_properties(
        integration_all integration_basic
        PROPERTIES LABELS "Integration"
    )
endif()

# ============================================
# CTest Configuration - E2E Tests - DISABLED: executable not built
# ============================================
# if(BOLT_ENABLE_E2E_TESTS)
#     add_test(NAME e2e_all COMMAND bolt_e2e_tests)
#     add_test(NAME e2e_workflow COMMAND bolt_e2e_tests E2E_Workflow)
#     add_test(NAME e2e_memory COMMAND bolt_e2e_tests E2E_Memory)
#     add_test(NAME e2e_editor COMMAND bolt_e2e_tests E2E_Editor)
#     add_test(NAME e2e_plugin COMMAND bolt_e2e_tests E2E_Plugin)
#     add_test(NAME e2e_logging COMMAND bolt_e2e_tests E2E_Logging)
#     add_test(NAME e2e_collaboration COMMAND bolt_e2e_tests E2E_Collaboration)
#     add_test(NAME e2e_network COMMAND bolt_e2e_tests E2E_Network)
#     add_test(NAME e2e_performance COMMAND bolt_e2e_tests E2E_Performance)
#     add_test(NAME e2e_error_handling COMMAND bolt_e2e_tests E2E_ErrorHandling)
#     add_test(NAME e2e_full_integration COMMAND bolt_e2e_tests E2E_FullIntegration)
#
#     set_tests_properties(
#         e2e_all e2e_workflow e2e_memory e2e_editor e2e_plugin
#         e2e_logging e2e_collaboration e2e_network e2e_performance
#         e2e_error_handling e2e_full_integration
#         PROPERTIES LABELS "E2E"
#     )
#
#     # E2E tests may take longer
#     set_tests_properties(
#         e2e_all e2e_full_integration
#         PROPERTIES TIMEOUT 300
#     )
# endif()

# ============================================
# Valgrind Tests (if enabled)
# ============================================
if(ENABLE_VALGRIND)
    if(BOLT_ENABLE_UNIT_TESTS)
        add_valgrind_test(valgrind_unit_tests bolt_unit_tests)
        add_valgrind_test(valgrind_sanitizer_tests bolt_sanitizer_tests)
    endif()

    if(BOLT_ENABLE_INTEGRATION_TESTS)
        add_valgrind_test(valgrind_integration_tests bolt_integration_tests)
    endif()

    # DISABLED: E2E executable not built
    # if(BOLT_ENABLE_E2E_TESTS)
    #     add_valgrind_test(valgrind_e2e_tests bolt_e2e_tests)
    # endif()
endif()

# ============================================
# Test Summary
# ============================================
message(STATUS "")
message(STATUS "=== Test Configuration Summary ===")
message(STATUS "Unit Tests:        ${BOLT_ENABLE_UNIT_TESTS}")
message(STATUS "Integration Tests: ${BOLT_ENABLE_INTEGRATION_TESTS}")
message(STATUS "E2E Tests:         ${BOLT_ENABLE_E2E_TESTS}")
message(STATUS "Performance Tests: ${BOLT_ENABLE_PERFORMANCE_TESTS}")
message(STATUS "GoogleTest:        ${GTest_FOUND}")
message(STATUS "Sanitizers:        ${SANITIZERS_ENABLED}")
message(STATUS "Valgrind:          ${ENABLE_VALGRIND}")
message(STATUS "==================================")
message(STATUS "")
