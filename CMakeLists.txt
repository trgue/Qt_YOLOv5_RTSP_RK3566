cmake_minimum_required(VERSION 3.5)

project(qtyolov5 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
# 更换为自己的路径
set(Qt5_DIR "/home/trgue/tool/qt5.12.10-arm64/lib/cmake/Qt5")
include_directories(/home/trgue/tool/opencv-3.4.1/install/include)
link_directories(/home/trgue/tool/opencv-3.4.1/install/lib)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/YOLOv5/cpp/ yolov5.out)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)

FILE(GLOB UI_FILES "mainwindow.ui") 
qt5_wrap_ui(WRAP_FILES ${UI_FILES})

add_executable(${PROJECT_NAME}
  main.cpp
  mainwindow.cpp
  mainwindow.h
  ${WRAP_FILES}
)


target_link_libraries(${PROJECT_NAME} rknn_yolov5)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
target_link_libraries(${PROJECT_NAME} Qt5::Gui)
target_link_libraries(${PROJECT_NAME} Qt5::Core)
target_link_libraries(${PROJECT_NAME} Qt5::Network)
target_link_libraries(${PROJECT_NAME} libopencv_core.so)
target_link_libraries(${PROJECT_NAME} libopencv_highgui.so)
target_link_libraries(${PROJECT_NAME} libopencv_imgproc.so)
target_link_libraries(${PROJECT_NAME} libopencv_videoio.so)
target_link_libraries(${PROJECT_NAME} libopencv_imgcodecs.so)

install(TARGETS ${PROJECT_NAME} DESTINATION .)

