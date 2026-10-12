cmake_minimum_required(VERSION 3.15)
project(eaglegui_native)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Qt6 (preferohet) ose Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Rruga e node-addon-api
execute_process(
  COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR "${NODE_ADDON_API_DIR}")

add_library(${PROJECT_NAME} SHARED src/main.cpp ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE
  NAPI_VERSION=8
  NAPI_DISABLE_CPP_EXCEPTIONS
  QT_NO_KEYWORDS
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB} Qt${QT_VERSION_MAJOR}::Widgets)
