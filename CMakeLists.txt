# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# --- Git Version Injection ---
# Extract the latest semantic Git tag (e.g. v0.0.5) and feed it into
# project(VERSION) so that ESP-IDF embeds it in esp_app_desc_t.
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_RESULT
    )
    if(GIT_RESULT EQUAL 0)
        string(REGEX MATCH "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${GIT_TAG}")
        if(CMAKE_MATCH_COUNT EQUAL 3)
            set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
            set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
            set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
            set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
        else()
            set(PROJECT_VERSION "0.0.0")
        endif()
    else()
        set(PROJECT_VERSION "0.0.0")
    endif()
else()
    set(PROJECT_VERSION "0.0.0")
endif()

# Emit a plain-text version descriptor for the CDN (S3 upload)
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${PROJECT_VERSION}")
message(STATUS "Firmware version from git: ${PROJECT_VERSION}")
# --- End Git Version Injection ---

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_OTA VERSION ${PROJECT_VERSION})
