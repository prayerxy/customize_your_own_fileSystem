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
dd if=/dev/zero of=test.img bs=1M count=256
echo "测试目录：/mnt/test"
echo "镜像名: test.img"
echo "镜像大小: 256MB"
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
sudo mount -o loop -t XCraft test.img /mnt/test
echo "测试镜像挂载到/mnt/test! "
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
sudo su <<EOF
cd /mnt/test
mkdir -p 0
cd 0
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 1
cd 1
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 2
cd 2
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 3
cd 3
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 4
cd 4
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 5
cd 5
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 6
cd 6
touch nku.txt
echo "hello world" > nku.txt
cd ..
ls
mkdir -p 7
cd 7
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 8
cd 8
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 9
cd 9
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 10
cd 10
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 11
cd 11
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 12
cd 12
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 13
cd 13
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 14
cd 14
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 15
cd 15
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 16
cd 16
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 17
cd 17
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 18
cd 18
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 19
cd 19
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 20
cd 20
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 21
cd 21
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 22
cd 22
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 23
cd 23
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 24
cd 24
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 25
cd 25
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 26


cd 26
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 27
cd 27
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 28
cd 28
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 29
cd 29
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 30
cd 30
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 31
cd 31
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 32
cd 32
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 33
cd 33
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 34
cd 34
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 35
cd 35
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 36
cd 36
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 37
cd 37
touch nku.txt
echo "hello world" > nku.txt
cd ..
mkdir -p 38
cd 38
touch nku.txt
echo "hello world" > nku.txt
cd ..

ls

cd 0
rm nku.txt
cd ..
rm -d 0
ls
cd 1
rm nku.txt
cd ..
rm -d 1
ls
cd 2
rm nku.txt
cd ..
rm -d 2
ls
cd 3
rm nku.txt
cd ..
rm -d 3
ls
cd 4
rm nku.txt
cd ..
rm -d 4
ls
cd 5
rm nku.txt
cd ..
rm -d 5
ls
cd 6
rm nku.txt
cd ..
rm -d 6
ls
cd 7
rm nku.txt
cd ..
rm -d 7
ls
cd 8
rm nku.txt
cd ..
rm -d 8
ls
cd 9
rm nku.txt
cd ..
rm -d 9
ls
cd 10
rm nku.txt
cd ..
rm -d 10
ls
cd 11
rm nku.txt
cd ..
rm -d 11
ls
cd 12
rm nku.txt
cd ..
rm -d 12
ls
cd 13
rm nku.txt
cd ..
rm -d 13
ls
cd 14
rm nku.txt
cd ..
rm -d 14
ls
cd 15
rm nku.txt
cd ..
rm -d 15
ls
cd 16
rm nku.txt
cd ..
rm -d 16
ls
cd 17
rm nku.txt
cd ..
rm -d 17
ls
cd 18
rm nku.txt
cd ..
rm -d 18
ls
cd 19
rm nku.txt
cd ..
rm -d 19
ls
cd 20
rm nku.txt
cd ..
rm -d 20
ls
cd 21
rm nku.txt
cd ..
rm -d 21
cd 22
rm nku.txt
cd ..
rm -d 22
cd 23
rm nku.txt
cd ..
rm -d 23
cd 24
rm nku.txt
cd ..
rm -d 24
cd 25
rm nku.txt
cd ..
rm -d 25
cd 26
rm nku.txt
cd ..
rm -d 26
cd 27
rm nku.txt
cd ..
rm -d 27
cd 28
rm nku.txt
cd ..
rm -d 28
cd 29
rm nku.txt
cd ..
rm -d 29
cd 30
rm nku.txt
cd ..
rm -d 30
cd 31
rm nku.txt
cd ..
rm -d 31
cd 32
rm nku.txt
cd ..
rm -d 32
cd 33
rm nku.txt
cd ..
rm -d 33
cd 34
rm nku.txt
cd ..
rm -d 34
cd 35
rm nku.txt
cd ..
rm -d 35
cd 36
rm nku.txt
cd ..
rm -d 36
cd 37
rm nku.txt
cd ..
rm -d 37
cd 38
rm nku.txt
cd ..
rm -d 38
EOF

# 卸载测试镜像
# 卸载 kernel mount point 和 module
echo "开始卸载 kernel mount point 和 module..."
sudo umount /mnt/test
sudo rmmod xcraft
echo "kernel mount point 和 module 卸载完成！"
echo -e "\n"
sleep 1
echo "开始清理构建环境..."
make clean
echo "构建环境清理完成！"

