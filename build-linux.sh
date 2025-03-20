#!/bin/bash
# 编译器路径，需要自己定义
GCC_COMPILER=/usr/bin/aarch64-linux-gnu

set -e

echo "$0 $@"

echo "$GCC_COMPILER"
export CC=${GCC_COMPILER}-gcc
export CXX=${GCC_COMPILER}-g++

if command -v ${CC} >/dev/null 2>&1; then
    :
else
    echo "${CC} is not available"
    echo "Please set GCC_COMPILER for $TARGET_SOC"
    echo "such as export GCC_COMPILER=~/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf"
    exit
fi


# 设置install目录
ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )
echo "ROOT_PWD: ${ROOT_PWD}"
INSTALL_DIR=${ROOT_PWD}/install
if [[ -d "${INSTALL_DIR}" ]]; then rm -rf ${INSTALL_DIR}
fi


cd build
cmake ../ \
    -DTARGET_SOC=rk356x \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_ASAN=OFF \
    -DDISABLE_RGA=OFF \
    -DDISABLE_LIBJPEG=OFF \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
make -j4
make install

# Check if there is a rknn model in the install directory
suffix=".rknn"
shopt -s nullglob
if [ -d "$INSTALL_DIR" ]; then
    files=("$INSTALL_DIR/model/"/*"$suffix")
    shopt -u nullglob

    if [ ${#files[@]} -le 0 ]; then
        echo -e "\e[91mThe RKNN model can not be found in \"$INSTALL_DIR/model\", please check!\e[0m"
    fi
else
    echo -e "\e[91mInstall directory \"$INSTALL_DIR\" does not exist, please check!\e[0m"
fi


