#!/bin/bash

# 编译 kernel module 和 tool
echo "开始编译 kernel module 和 tool..."
make
echo "编译完成！"
echo -e "\n"
sleep 1

# 加载 kernel module
echo "开始加载 kernel module..."
cd build
sudo insmod xcraft.ko
echo "kernel module 加载完成！"
echo -e "\n"
sleep 1

cd ..
# 创建测试目录和测试镜像
echo "开始创建测试目录和测试镜像..."
sudo mkdir -p /mnt/test

dd if=/dev/zero of=test.img bs=1M count=1024
echo "测试目录：/mnt/test"
echo "镜像名: test.img"
echo "镜像大小: 1GB"
echo -e "\n"
sleep 1


# 使用mkfs.XCraft工具创建文件系统
echo "开始使用 mkfs.XCraft 工具创建文件系统..."
./build/mkfs.XCraft test.img
echo "文件系统test.img创建完成! "
echo -e "\n"
sleep 1

# 挂载测试镜像
echo "开始挂载测试镜像..."
# sudo mount -o loop -t XCraft test.img /mnt/test
sudo mount -o loop -t XCraft test.img /mnt/test
echo "测试镜像挂载到/mnt/test! "
sudo chmod -R 777 /mnt/test
echo -e "\n"
sleep 1

#显示文件系统情况
echo "显示文件系统情况..."
df -TH
echo -e "\n"
sleep 1
# 检查挂载成功的 kernel 消息
#dmesg | tail

# 执行一些文件系统操作
echo "xxxxxxxxxxxxxxxxxxxxxx开始执行文件系统操作xxxxxxxxxxxxxxxxxxxxxxxx"
#使用make test测试