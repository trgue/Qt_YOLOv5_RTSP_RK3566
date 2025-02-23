cmake_minimum_required(VERSION 3.10)

project(rknn_yolov5)

set(rknpu_yolov5_file rknpu2/yolov5.cc)


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../utils/ utils.out)

# ORIGIN = LD_LIBRARY_PATH
# export LD_LIBRARY_PATH=./lib
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")


# 递归查询.cc文件，保存在SRCS变量中
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(rknn_yolov5
    yolo.cc
    postprocess.cc
    ${rknpu_yolov5_file}
)

target_link_libraries(rknn_yolov5
    imageutils
    fileutils
    imagedrawing    
    ${LIBRKNNRT}
    dl
)


if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(rknn_yolov5 Threads::Threads)
endif()

target_include_directories(rknn_yolov5 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBRKNNRT_INCLUDES}
)

# install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/bus.jpg DESTINATION ./model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/coco_80_labels_list.txt DESTINATION ./model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)