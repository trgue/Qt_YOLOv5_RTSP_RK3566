cmake_minimum_required(VERSION 3.5)

project(cmaketest LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# set (CMAKE_C_COMPILER "/home/trgue/rknn/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc")
# set (CMAKE_CXX_COMPILER "/home/trgue/rknn/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++")
# set (CMAKE_C_COMPILER "/usr/bin/aarch64-linux-gnu-gcc")
# set (CMAKE_CXX_COMPILER "/usr/bin/aarch64-linux-gnu-g++")


# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/YOLOv5/cpp/ yolov5.out)

set(Qt5_DIR /home/trgue/tool/qt5.12.10-arm64/lib/cmake/Qt5)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
include_directories("/home/trgue/tool/opencv-3.4.1/install/include")
link_directories(/home/trgue/tool/opencv-3.4.1/install/lib)


if(ANDROID)
  add_library(cmaketest SHARED
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
  )
else()
  add_executable(cmaketest
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
  )
endif()


target_link_libraries(cmaketest rknn_yolov5)
target_link_libraries(cmaketest Qt5::Widgets)
target_link_libraries(cmaketest libopencv_core.so)
target_link_libraries(cmaketest libopencv_highgui.so)
target_link_libraries(cmaketest libopencv_imgproc.so)
target_link_libraries(cmaketest libopencv_videoio.so)
target_link_libraries(cmaketest libopencv_imgcodecs.so)

