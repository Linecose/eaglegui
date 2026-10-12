cmake_minimum_required(VERSION 3.15)
project(eaglegui_native)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 1. Gjej paketën Qt6 ose Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 2. Gjej shtegun e node-addon-api
execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX REPLACE "^\"|\"$" "" NODE_ADDON_API_DIR "${NODE_ADDON_API_DIR}")

# 3. Gjej shtegun e skedarëve header të vetë Node.js (zgjidh gabimin node_api.h)
execute_process(
    COMMAND node -p "require('node-api-headers').include || ''"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_API_HEADERS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Krijimi i modulit nativ
add_library(${PROJECT_NAME} SHARED main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Përfshirja e dosjeve me headers
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_JS_INC} 
    ${NODE_ADDON_API_DIR}
    ${NODE_API_HEADERS_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    NAPI_VERSION=8
    NAPI_DISABLE_CPP_EXCEPTIONS
    QT_NO_KEYWORDS
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB} Qt${QT_VERSION_MAJOR}::Widgets)
