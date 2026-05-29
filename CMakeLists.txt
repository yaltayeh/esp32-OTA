# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_OTA)

add_custom_target(deploy ALL
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/deploy_update.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Deploying firmware update to server..."
)

add_dependencies(deploy app)
