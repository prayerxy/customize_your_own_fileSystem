# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
TEST_DIR := test

# Compiler and flags
CC := gcc
CFLAGS := -std=gnu99 -Wall -I$(INC_DIR) -I$(SRC_DIR)

# Object files
obj-m += xcraft.o
ccflags-y := -I$(PWD)/$(INC_DIR)
xcraft-objs := $(patsubst %.o, $(SRC_DIR)/%.o, fs.o super.o inode.o file.o dir.o)

KDIR ?= /lib/modules/$(shell uname -r)/build

MKFS = $(BUILD_DIR)/mkfs.XCraft

run: $(BUILD_DIR) $(MKFS)
	make -C $(KDIR) M=$(PWD) modules
	mv *.symvers $(BUILD_DIR)
	mv *.ko $(BUILD_DIR)
	mv *.mod.c $(BUILD_DIR)
	mv *.mod.o $(BUILD_DIR)
	mv *.o $(BUILD_DIR)
	mv *.order $(BUILD_DIR)
	mv *.mod $(BUILD_DIR)
	mv .Module.symvers.cmd $(BUILD_DIR)
	mv .modules.order.cmd $(BUILD_DIR)
	mv .*.ko.cmd $(BUILD_DIR)
	mv .*.mod.cmd $(BUILD_DIR)
	mv .*.mod.o.cmd $(BUILD_DIR)
	mv .*.o.cmd $(BUILD_DIR)
	mv $(SRC_DIR)/*.o $(BUILD_DIR)
	mv $(SRC_DIR)/.*.o.cmd $(BUILD_DIR)

IMAGE ?= test.img
IMAGESIZE ?= 256

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	chmod 755 $(BUILD_DIR)

$(MKFS): $(SRC_DIR)/mkfs.c
	$(CC) $(CFLAGS) -o $@ $<
	chmod 755 $(MKFS)

$(IMAGE): $(MKFS)
	dd if=/dev/zero of=${IMAGE} bs=1M count=${IMAGESIZE}
	./$< $(IMAGE)

# Compile all test files in /test directory
$(BUILD_DIR)/%: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

# Generate list of built tests
BUILD_TESTS := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(wildcard $(TEST_DIR)/*.c))

# Run all tests or a specific test
test: $(BUILD_DIR) $(BUILD_TESTS)
ifeq ($(filter-out $@,$(MAKECMDGOALS)),)
	@for test in $(BUILD_TESTS); do \
		sudo $$test $(IMAGE); \
	done
else
	@k=$(filter-out $@,$(MAKECMDGOALS)); \
	if [ $$k -gt 0 ] && [ $$k -le $(words $(BUILD_TESTS)) ]; then \
		TEST=$$(echo $(BUILD_TESTS) | tr ' ' '\n' | sed -n "$$k"p); \
		sudo $$TEST $(IMAGE); \
	else \
		echo "Test number $$k is out of range."; \
	fi
endif

# Check the image
check: all
	script/test.sh $(IMAGE) $(IMAGESIZE) $(MKFS)

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *~ $(PWD)/*.ur-safe
	rm -f $(MKFS) $(IMAGE)
	rm -rf $(BUILD_DIR)

.PHONY: all clean test check

# Prevent make from treating 'k' as a file name
%:
	@:
