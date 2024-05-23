# Directories
SRC_DIR := src
INC_DIR := include
BUILDDIR := build

# Compiler and flags
CC := gcc
CFLAGS := -std=gnu99 -Wall -I$(INC_DIR) -I$(SRC_DIR)

# Object files
obj-m += xcraft.o
ccflags-y := -I$(PWD)/$(INC_DIR)
xcraft-objs := $(patsubst %.o, $(SRC_DIR)/%.o, fs.o super.o inode.o file.o dir.o)



KDIR ?= /lib/modules/$(shell uname -r)/build

MKFS = $(BUILDDIR)/mkfs.XCraft

all: $(BUILDDIR) $(MKFS)
	make -C $(KDIR) M=$(PWD) modules
	mv *.symvers $(BUILDDIR)
	mv *.ko $(BUILDDIR)
	mv *.mod.c $(BUILDDIR)
	mv *.mod.o $(BUILDDIR)
	mv *.o $(BUILDDIR)
	mv *.order $(BUILDDIR)
	mv *.mod $(BUILDDIR)
	mv .Module.symvers.cmd $(BUILDDIR)
	mv .modules.order.cmd $(BUILDDIR)
	mv .*.ko.cmd $(BUILDDIR)
	mv .*.mod.cmd $(BUILDDIR)
	mv .*.mod.o.cmd $(BUILDDIR)
	mv .*.o.cmd $(BUILDDIR)

	mv $(SRC_DIR)/*.o $(BUILDDIR)
	mv $(SRC_DIR)/.*.o.cmd $(BUILDDIR)

IMAGE ?= test.img
IMAGESIZE ?= 256
# To test max files(40920) in directory, the image size should be at least 159.85 MiB
# 40920 * 4096(block size) ~= 159.85 MiB

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(MKFS): $(SRC_DIR)/mkfs.c
	$(CC) $(CFLAGS) -o $@ $<

$(IMAGE): $(MKFS)
	dd if=/dev/zero of=${IMAGE} bs=1M count=${IMAGESIZE}
	./$< $(IMAGE)

check: all
	script/test.sh $(IMAGE) $(IMAGESIZE) $(MKFS)

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *~ $(PWD)/*.ur-safe
	rm -f $(MKFS) $(IMAGE)
	rm -rf $(BUILDDIR)
.PHONY: all clean

