cmake_minimum_required(VERSION 3.15)

project(ffmpeg_encoder)

set(FFMPEG_DIR "/home/trgue/tool/ffmpeg-rockchip/ffmpeg_rk_build")
set(OPENCV_DIR "/home/trgue/tool/opencv-3.4.1/install")
set(DRM_DIR "/home/trgue/tool/libdrm-2.4.107/install")
set(ROCKCHIP_MPP_DIR "/home/trgue/tool/rkmpp/install")

set(FFMPEG_INCLUDE_DIR ${FFMPEG_DIR}/include)
set(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib)
set(OPENCV_INCLUDE_DIR ${OPENCV_DIR}/include)
set(OPENCV_LIB_DIR ${OPENCV_DIR}/lib)
set(DRM_INCLUDE_DIR ${DRM_DIR}/include)
set(DRM_LIB_DIR ${DRM_DIR}/lib)
set(ROCKCHIP_MPP_INCLUDE_DIR ${ROCKCHIP_MPP_DIR}/include)
set(ROCKCHIP_MPP_LIB_DIR ${ROCKCHIP_MPP_DIR}/lib)
set(X264_LIB_DIR "/home/trgue/tool/x264/x264build/lib")

set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

add_library(${PROJECT_NAME}
    encoder.cpp
)

target_link_directories(${PROJECT_NAME}
    PUBLIC
        ${OPENCV_LIB_DIR}
        ${FFMPEG_LIB_DIR}
        ${DRM_LIB_DIR}
        ${ROCKCHIP_MPP_LIB_DIR}
        ${X264_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        avcodec
        avformat
        avutil
        swscale
        swresample
        avfilter
        avdevice
        rockchip_mpp
    PUBLIC
        opencv_core
        opencv_highgui
        opencv_imgproc
        opencv_videoio
        opencv_imgcodecs
        pthread
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OPENCV_INCLUDE_DIR}
    PRIVATE
        ${FFMPEG_INCLUDE_DIR}
        ${DRM_INCLUDE_DIR}
        ${ROCKCHIP_MPP_INCLUDE_DIR}
)


