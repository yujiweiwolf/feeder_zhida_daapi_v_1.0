# cmake_minimum_required(VERSION <specify CMake version here>)
cmake_minimum_required(VERSION 3.19)
project(lessdb)

set(CMAKE_CXX_STANDARD 17)
SET(ROOT_DIR "/home/work/sys/lib")
#Release Debug
set(CMAKE_BUILD_TYPE "Release")

#find_package(X REQUIRED)
#include_directories(${X_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/DAAPI_v1.15/include)
link_directories(${CMAKE_CURRENT_LIST_DIR}/lib/DAAPI_v1.15/Linux/gcc5.4.0-6)

add_executable(zhida_daapi_feeder
        src/main.cc
        src/md_buffer.h
        src/md_callback.cc
        src/config.h
        src/daapi_server.cc
        src/define.h
        src/md_buffer.cc
        src/daapi_server.h
        src/md_callback.h
        src/config.cc
        )

target_link_libraries(zhida_daapi_feeder
        DAApi feeder coral swordfish x stdc++fs yaml-cpp clickhouse-cpp-lib-static boost_program_options boost_iostreams z protobuf pthread boost_date_time boost_filesystem boost_regex boost_system  boost_chrono boost_log boost_program_options boost_thread boost_iostreams z protobuf protobuf-lite sodium zmq ssl crypto iconv pthread dl clickhouse-cpp-lib-static)

FILE(GLOB LIBS_NAME "lib/DAAPI_v1.15/Linux/gcc5.4.0-6/*")
MESSAGE("复制库文件 ${LIBS_NAME}")
FILE(COPY ${LIBS_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB COMMON_LIB_NAME "${ROOT_DIR}/common_lib/lib/*so*")
MESSAGE("复制公共库文件 ${COMMON_LIB_NAME}")
FILE(COPY ${COMMON_LIB_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


FILE(COPY Dockerfile DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/image.sh "n=$1; v=`./$n -v`; ldd $n | grep /sys/lib/ | awk '{print $3}' | xargs -i cp -L -n {} .; docker build -t $n:$v .; rm -rf \${n}_$v.docker.tar.gz; docker save $n:$v > \${n}_$v.docker.tar && gzip \${n}_$v.docker.tar")

add_custom_target(image COMMAND sh image.sh zhida_daapi_feeder DEPENDS zhida_daapi_feeder)
