#!/bin/bash

# 编译 kernel module 和 tool
echo "开始编译 kernel module 和 tool..."
make
echo "编译完成！"
echo -e "\n"
sleep 1

# 加载 kernel module

echo "开始加载 kernel module..."
sudo insmod xcraft.ko
echo "kernel module 加载完成！"
echo -e "\n"
sleep 1

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
./mkfs.XCraft test.img
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
mkdir -p dir0
cd dir0
mkdir -p dir1
mkdir -p dir2
mkdir -p dir3
mkdir -p dir4
mkdir -p dir5
mkdir -p dir6
mkdir -p dir7
mkdir -p dir8
mkdir -p dir9
mkdir -p dir10
mkdir -p dir11
mkdir -p dir12
mkdir -p dir13
mkdir -p dir14
mkdir -p dir15
mkdir -p dir16

mkdir -p dir17
mkdir -p dir18
mkdir -p dir19
mkdir -p dir20
mkdir -p dir21
mkdir -p dir22
mkdir -p dir23
mkdir -p dir24
mkdir -p dir25
mkdir -p dir26
mkdir -p dir27
mkdir -p dir28
mkdir -p dir29
mkdir -p dir30
mkdir -p dir31
mkdir -p dir32
mkdir -p dir33
mkdir -p dir34
mkdir -p dir35

touch 1.txt
ls
cd dir1
touch 2.txt
echo "zjr is so handsome, no one can exceed him" > 2.txt
cat 2.txt
ls
rm 2.txt
cd ..
rm 1.txt
rm -d dir1
rm -d dir2
rm -d dir3
rm -d dir4
rm -d dir5
rm -d dir6
rm -d dir7
rm -d dir8
rm -d dir9
rm -d dir10
rm -d dir11
ls
rm -d dir12
rm -d dir13
rm -d dir14
rm -d dir15
rm -d dir16
rm -d dir17
rm -d dir18
rm -d dir19
rm -d dir20
rm -d dir21
rm -d dir22
rm -d dir23
rm -d dir24
rm -d dir25
rm -d dir26
rm -d dir27
rm -d dir28
rm -d dir29
rm -d dir30
rm -d dir31
rm -d dir32
rm -d dir33
rm -d dir34
rm -d dir35
cd ..
ls
rm -d dir0
EOF

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

