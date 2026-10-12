cmake_minimum_required(VERSION 3.15)
project(eaglegui_native)

# Përcaktimi i standardit C++17 për Qt6
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Gjej Qt6 / Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Gjej node-addon-api
execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX REPLACE "^\"|\"$" "" NODE_ADDON_API_DIR "${NODE_ADDON_API_DIR}")

add_library(${PROJECT_NAME} SHARED main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_JS_INC} 
    ${NODE_ADDON_API_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    NAPI_VERSION=8
    NAPI_DISABLE_CPP_EXCEPTIONS
    QT_NO_KEYWORDS
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB} Qt${QT_VERSION_MAJOR}::Widgets)
