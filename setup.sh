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
mkdir -p dir36
mkdir -p dir37
mkdir -p dir38
mkdir -p dir39
mkdir -p dir40
mkdir -p dir41
mkdir -p dir42
mkdir -p dir43
mkdir -p dir44
mkdir -p dir45
mkdir -p dir46
mkdir -p dir47
mkdir -p dir48
mkdir -p dir49
mkdir -p dir50
mkdir -p dir51
mkdir -p dir52
mkdir -p dir53
mkdir -p dir54
mkdir -p dir55
mkdir -p dir56
mkdir -p dir57
mkdir -p dir58
mkdir -p dir59
mkdir -p dir60
mkdir -p dir61
mkdir -p dir62
mkdir -p dir63
mkdir -p dir64
mkdir -p dir65
mkdir -p dir66
mkdir -p dir67
mkdir -p dir68
mkdir -p dir69
mkdir -p dir70
mkdir -p dir71
mkdir -p dir72
mkdir -p dir73
mkdir -p dir74
mkdir -p dir75
mkdir -p dir76
mkdir -p dir77
mkdir -p dir78
mkdir -p dir79
mkdir -p dir80
mkdir -p dir81
mkdir -p dir82
mkdir -p dir83
mkdir -p dir84
mkdir -p dir85
mkdir -p dir86
mkdir -p dir87
mkdir -p dir88
mkdir -p dir89
mkdir -p dir90
mkdir -p dir91
mkdir -p dir92
touch 1.txt
echo "zjr is a super man, no one can exceed him!" > 1.txt
cat 1.txt
ls
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
rm -d dir36
rm -d dir37
rm -d dir38
rm -d dir39
rm -d dir40
rm -d dir41
rm -d dir42
rm -d dir43
rm -d dir44
rm -d dir45
rm -d dir46
rm -d dir47
rm -d dir48
rm -d dir49
rm -d dir50
rm -d dir51
rm -d dir52
rm -d dir53
rm -d dir54
rm -d dir55
rm -d dir56
rm -d dir57
rm -d dir58
rm -d dir59
rm -d dir60
rm -d dir61
rm -d dir62
rm -d dir63
rm -d dir64
rm -d dir65
rm -d dir66
rm -d dir67
rm -d dir68
rm -d dir69
rm -d dir70
rm -d dir71
rm -d dir72
rm -d dir73
rm -d dir74
rm -d dir75
rm -d dir76
rm -d dir77
rm -d dir78
rm -d dir79
rm -d dir80
rm -d dir81
rm -d dir82
rm -d dir83
rm -d dir84
rm -d dir85
rm -d dir86
rm -d dir87
ls
rm -d dir88
rm -d dir89
rm -d dir90
rm -d dir91
rm -d dir92
rm 1.txt
cd ..
rm -d dir0

mkdir -p dir1
cd dir1
touch 1.txt
ls
cd ..
mkdir -p dir2
mv /mnt/test/dir1/1.txt /mnt/test/dir2/
cd dir1
ls
cd ..
cd dir2
ls
rm 1.txt
cd ..
rm -d dir1
rm -d dir2
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

