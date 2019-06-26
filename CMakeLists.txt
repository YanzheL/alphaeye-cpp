cmake_minimum_required(VERSION 3.5)

project(alphaeye_cpp)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

find_package(raspicam REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

message("CMAKE_C_FLAGS_DEBUG is ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE is ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_C_FLAGS_RELWITHDEBINFO is ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
message("CMAKE_C_FLAGS_MINSIZEREL is ${CMAKE_C_FLAGS_MINSIZEREL}")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")

add_executable(
        alphaeye_cpp
        main.cc
        detector/motion_detector.cc
        detector/motion_detector.h
        detector/bgs_motion_detector.cc
        detector/bgs_motion_detector.h
        util/util.h
        io_node/video_output_node.cc
        io_node/video_output_node.h
        controller/detector_controller.cc
        controller/detector_controller.h
        io_node/db_writer.cc io_node/db_writer.h
)

target_link_libraries(
        alphaeye_cpp
        opencv_world
        pthread
        ${Boost_LIBRARIES}
        ${raspicam_CV_LIBS}
        tbb
        sqlite3
)