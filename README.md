## XCRAFT_FS

我们来自南开大学的队伍PRAY4U

选题题目是`proj209-自己设计一个Linux 文件系统并实现文件和目录读写操作`

| 小组成员 | 谢畅、张景瑞 |
| -------- | ------------ |
| 指导老师 | 宫晓利、张金 |

### 提交记录说明

- prayerxy、谢畅均为谢畅提交
- cmzjr 是张景瑞提交

### 项目的结构说明

- data_about_learning是准备阶段完成的学习记录
- 其余文件夹是项目结构
  - include是头文件夹
  - src是项目源文件夹
  - 外部makefile保证运行make
  - setup.sh是运行测试脚本

### 运行说明

- 方法1：在项目根目录下直接运行脚本

  ```
  sudo bash setup.sh
  
  ```

  演示效果:

  ```
  显示文件系统情况...
  文件系统       类型    大小  已用  可用 已用% 挂载点
  tmpfs          tmpfs   406M  2.2M  404M    1% /run
  /dev/sda3      ext4     31G   26G  4.3G   86% /
  tmpfs          tmpfs   2.1G     0  2.1G    0% /dev/shm
  tmpfs          tmpfs   5.3M  4.1k  5.3M    1% /run/lock
  tmpfs          tmpfs   2.1G     0  2.1G    0% /run/qemu
  /dev/sda2      vfat    537M  6.4M  531M    2% /boot/efi
  tmpfs          tmpfs   406M  103k  406M    1% /run/user/1000
  /dev/loop17    XCraft  269M  1.1M  268M    1% /mnt/test
  
  xxxxxxxxxxxxxxxxxxxxxx开始执行文件系统操作xxxxxxxxxxxxxxxxxxxxxxxx
  it's a test! it's a test! it's a test! it's a test
  1.txt  dir13  dir18  dir22  dir27  dir31  dir36  dir40  dir45  dir5   dir54  dir59  dir63  dir68  dir72  dir77  dir81  dir86  dir90
  dir1   dir14  dir19  dir23  dir28  dir32  dir37  dir41  dir46  dir50  dir55  dir6   dir64  dir69  dir73  dir78  dir82  dir87  dir91
  dir10  dir15  dir2   dir24  dir29  dir33  dir38  dir42  dir47  dir51  dir56  dir60  dir65  dir7   dir74  dir79  dir83  dir88  dir92
  dir11  dir16  dir20  dir25  dir3   dir34  dir39  dir43  dir48  dir52  dir57  dir61  dir66  dir70  dir75  dir8   dir84  dir89
  dir12  dir17  dir21  dir26  dir30  dir35  dir4   dir44  dir49  dir53  dir58  dir62  dir67  dir71  dir76  dir80  dir85  dir9
  1.txt  dir88  dir89  dir90  dir91  dir92
  1.txt
  1.txt
  开始卸载 kernel mount point 和 module...
  kernel mount point 和 module 卸载完成！
  ```

  

- 方法2：也可以手动运行，先Make编译部署至linux内核，再挂载至一个文件夹下，对文件系统测试


### 项目目录

```
├─ data_about_learning # 准备阶段学习记录
├─ include
│  ├─ bitmap.h      # 位图操作
│  ├─ hash.h        # 目录哈希操作
│  ├─ XCRAFT.h      # 文件系统头文件
├─ src
│  ├─ dir.c         # 目录操作
│  ├─ file.c        # 文件操作
│  ├─ fs.c          # 文件系统注册挂载与卸载
│  ├─ inode.c       # inode操作
│  ├─ mkfs.c        # 文件系统格式化
│  ├─ super.c       # 超级块操作
├─ test
│  ├─ sample1.c       # 测试文件(待开发功能)
│  ├─ sample2.c       # 测试文件(待开发功能)
│  ├─ ...             
├─ Makefile        # 编译文件
├─ README.md
├─ setup.sh        # 测试脚本


```

### 初赛完成内容：

- 设计实现一个Linux内核模块，此模块完成如下功能：
  - 将新创建的文件系统的操作接口和VFS对接。
  - 实现新的文件系统的超级块、dentry、inode的读写操作。
  - 实现新的文件系统的权限属性，不同的用户不同的操作属性。
  - 实现和用户态程序的对接，用户程序
- 设计实现一个用户态应用程序，可以将一个块设备（可以用文件模拟）格式化成自己设计的文件系统的格式。
- 设计一个用户态的测试用例应用程序，测试验证自己的文件系统的open/read/write/ls/cd 等通常文件系统的访问。



项目适用于数据密集型应用场景，已经完成以下特性：

1. **延迟初始化，使用块组**，更好管理大储存的文件系统；在mkfs只需初始化第一个块组，之后第一个块组使用完毕后再初始化其他块组。性能较好
2. **循环块组的分配方式**。每个块组内部有独立的inode_bitmap以及block_bitmap，管理效率较于传统文件系统，更好。
3. 性能较好。数据密集型应用场景下，一个目录Inode往往会索引多个目录项，那么查找目录的时候性能就会很低，传统文件系统查找目录项是串行遍历查找；我们使用目录哈希的方式，在Inode下会检查目录项数量，当目录项数量超过一定限制后，立即构建**目录的哈希B+树**，大大提高了性能。







其他特性待完成：（决赛阶段）

- **inode索引大文件**，改用类似B+树的方式，避免由于大文件带来索引效率的下降，使用B+树会提升大文件的使用的效率(查找等效率提升)
- **日志校验和**，需要提高文件系统的**可靠性**。日志校验和进一步提高文件系统的可靠性和数据完整性，其确保了日志记录的完整性，使文件系统能够在崩溃或其他异常情况发生时进行更可靠的恢复。其原理简单概述便是在进行故障恢复时我们会重新计算校验和与日志中每条记录的校验和进行比较，如果一致便代表我们可以重放日志此记录操作；
- 对比传统文件系统，比如开源项目simplefs的性能，在大数据密集型场景对比文件系统的性能。
- 初赛已经完成4000+行的独立代码编写量，最后总计预计达到1万行