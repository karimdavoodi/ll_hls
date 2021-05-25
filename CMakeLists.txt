cmake_minimum_required (VERSION 3.15)
project (low_latency_hls)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions( 
                 -fdiagnostics-show-location=every-line
                 -DBOOST_LOG_DYN_LINK=1
                 -Wall
                 -g
                 )

########## Check Third party dependency
if( NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/avpp/av.h )
    message(FATAL_ERROR
        "Please clone and make 'avpp' in third_party folder: 
        git clone https://github.com/h4tr3d/avcpp.git ")
endif()
if( NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/redis-plus-plus/src/sw/redis++/redis++.h )
    message(FATAL_ERROR
        "Please clone and make 'redis++' in third_party folder: 
        git clone https://github.com/sewenew/redis-plus-plus.git")
endif()
if( NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/served/src/served/served.hpp )
    message(FATAL_ERROR
        "Please clone and make 'served' in third_party folder: 
        git clone https://github.com/meltwater/served.git") 
endif()

find_package(Boost COMPONENTS system log log_setup REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LibAV REQUIRED 
            libavdevice   
            libavformat  
            libavresample 
            libavfilter  
            libavcodec    
            libavutil     
            )
        #link_directories(
        #${CMAKE_SOURCE_DIR}/third_party/avpp
        #${CMAKE_SOURCE_DIR}/third_party/redis-plus-plus/build
        #${CMAKE_SOURCE_DIR}/third_party/served/lib
        #)
#############  hls_generator
add_executable(hls_generator src/hls_generator.cpp src/hls_generator_main.cpp)
target_include_directories(hls_generator  PUBLIC 
            third_party/avpp
            third_party/redis-plus-plus/src
            ${LibAV_INCLUDE_DIRS} 
            ${Boost_INCLUDE_DIRS}
            )
target_link_libraries (hls_generator 
            ${Boost_LIBRARIES} 
            ${LibAV_LIBRARIES} 
            )
#############  hls_server
add_executable(hls_server 
            src/redis_client.cpp 
            src/hls_server.cpp 
            src/hls_server_main.cpp
            )
target_include_directories(hls_server  PUBLIC 
            third_party/served/src/served
            third_party/redis-plus-plus/src
            third_party/redis-plus-plus/src/sw/redis++/no_tls
            ${Boost_INCLUDE_DIRS}
            )
target_link_libraries (hls_server 
            ${CMAKE_SOURCE_DIR}/third_party/hiredis/libhiredis.a
            ${CMAKE_SOURCE_DIR}/third_party/served/lib/libserved.a
            ${Boost_LIBRARIES} 
            pthread
            )

add_subdirectory(test)
enable_testing ()
add_test (NAME MyTest COMMAND Test)
