cmake_minimum_required(VERSION 3.6)
project(gpio_led_image)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(utils/CompileUtility)

set(CMAKE_CXX_STANDARD 14)

generate_flags(GPIOLI_COMPILE_FLAGS "native" 2 true)
list(APPEND CMAKE_CXX_FLAGS ${GPIOLI_COMPILE_FLAGS})

find_package(lambdacommon REQUIRED)
find_package(WiringPi REQUIRED)

include_directories(${LAMBDACOMMON_INCLUDE_DIRS} ${WIRINGPI_INCLUDE_DIRS})

add_executable(gpio_led_image main.cpp)
target_link_libraries(gpio_led_image AperLambda::lambdacommon drogon::WiringPi)
