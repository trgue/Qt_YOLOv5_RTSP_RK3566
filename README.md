# Qt_YOLOv5_RK3566

## Description 简介
基于[rknn_model_zoo](https://github.com/airockchip/rknn_model_zoo)示例中YOLOv5的一个Qt项目应用，采用Opencv读取摄像头视频，并显示到Qt界面上。

检测模型尚未进行改良，使用的还是开源项目中的原模型

* Platform: RK3566（泰山派开发板2G+16G版本）
* Qt version: 5.12.10
* Opencv version: 3.4.1
* environment: WSL2

## 设备连接
由于WSL2无法识别USB设备，之前一直是将WSL2中的文件copy到Windows再利用adb传到泰山派上，后来发现adbkit可以将USB设备模拟成tcp端口，然后再通过网络连接到WSL2。
### 1. 安装adbkit
首先在Windows上安装[node](https://nodejs.org/zh-cn/download)

```shell
# Download and install fnm:
winget install Schniz.fnm
# Download and install Node.js:
fnm install 22
# Verify the Node.js version:
node -v # Should print "v22.14.0".
# Verify npm version:
npm -v # Should print "10.9.2".
```
安装完成后记得把node添加到**环境变量**

然后通过npm安装adbkit
```shell
npm install --save adbkit
```

### 2. USB to TCP
首先查询Windows的IP，需要对应WSL的那个以太网
```shell
ipconfig
```
![alt text](image-1.png)

还要查询自己设备的ID号
```shell
adb devices
```

然后在Windows上将USB模拟为TCP端口
```shell
node your_path_to_adbkit usb-device-to-tcp your_adb_device_ID -p your_port
# eg. node node_modules/adbkit/bin/adbkit usb-device-to-tcp 913f75e3ec33c186 -p 8080
```

最后利用adb在WSL上连接到对应IP的端口，这样adb就可以连接到我们的开发板了:)
```shell
# connect to Windows
adb connect your_IP:your_port
# eg. adb connect 192.168.1.1:8080
# connect to board
adb shell
```

## Dependency library installation 依赖库安装

### Opencv

