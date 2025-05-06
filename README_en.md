## XCRAFT_FS

We are Team PRAY4U from Nankai University.

Our project is titled `proj209 - Design Your Own Linux File System and Implement File and Directory Read/Write Operations`.

We have designed a **lightweight, efficient, and easy-to-test data-intensive file system: XCRAFT_FS**.

![](./show.gif)

### Development Report

[Development Report](./XCRAFT开发文档.pdf)

### Demo Videos

<div>
    <b>Detailed Project Explanation Video: https://pan.baidu.com/s/1JFFDYpw5kEdWhu2zMRdqUg?pwd=1234</b><br>
    <b>Access Code: 1234</b><br>
</div>

<div>
    <b>Project Demo Video: https://pan.baidu.com/s/1_tJK7ZWdm3xWzMMhuUjxVQ?pwd=1234</b><br>
    <b>Access Code: 1234</b><br>
</div>

### References

- Referred to the ext4 source code, [ext4](https://github.com/torvalds/linux/tree/master/fs/ext4), to understand certain functionality principles.
- Referred to the simplefs framework to build our file system from scratch, [simplefs](https://github.com/sysprog21/simplefs/tree/master).
- Complies with the **GNU General Public License**.

### Project Structure Explanation

- `data_about_learning` contains learning records from the preparation phase.
- Other folders form the project structure:
  - `include`: header files
  - `src`: source code
  - External `Makefile` for build automation
  - `setup.sh`: test runner script
  - `4test.sh`: creates the filesystem and mounts it to `/mnt/test/`

### Run Instructions

- **Method 1**: Directly run the script in the project root directory

  ```bash
  bash setup.sh
  ```

  Sample Output:

  ```
  Displaying file system status...
  Filesystem      Type   Size  Used Avail Use% Mounted on
  tmpfs           tmpfs  406M  2.2M  404M   1% /run
  /dev/sda3       ext4    31G   26G  4.3G  86% /
  ...
  /dev/loop17     XCraft 269M  1.1M  268M   1% /mnt/test

  xxxxxxxxxxxxxxxxxxxxxx Starting file system operations xxxxxxxxxxxxxxxxxxxxx
  it's a test! it's a test! it's a test! it's a test
  1.txt  dir13  dir18  ...
  ...
  Unmounting kernel mount point and module...
  Unmount complete!
  ```

- **Method 2**: Manually compile and deploy into the Linux kernel with `make`, mount to a folder, and test the file system.

- **Test File System Performance**:

  ```bash
  bash 4test.sh
  make test
  ```

  Output:

  ```
  ####################
  file content: Hello World!
  file deleted
  file write performance: 0.005930
  ####################
  ####################
  already wrote 100 files
  already deleted 100 files
  file write performance: 0.007819
  ####################
  ```

### Project Directory

```
├─ data_about_learning   # Learning logs from the preparation phase
├─ include
│  ├─ bitmap.h      # Bitmap operations
│  ├─ extents.h     # Extent tree operations
│  ├─ hash.h        # Directory hash operations
│  ├─ XCRAFT.h      # File system header
├─ src
│  ├─ dir.c         # Directory operations
│  ├─ file.c        # File operations
│  ├─ fs.c          # File system registration and unmounting
│  ├─ inode.c       # Inode operations
│  ├─ mkfs.c        # File system formatting
│  ├─ super.c       # Superblock operations
├─ test
│  ├─ sample1.c     # Test file: read/write files
│  ├─ sample2.c     # Test file: create and delete 10000 directories and files
│  ├─ ...
├─ Makefile         # Build file
├─ README.md
├─ setup.sh         # Setup script
├─ 4test.sh         # Create file system, then test with `make test`
```

### Final Competition Achievements:

- Designed and implemented a Linux kernel module to:
  - Interface newly created file system operations with VFS.
  - Implement read/write operations for the superblock, dentry, and inode of the new file system.
  - Implement permission attributes for the new file system, ensuring different users have different access rights.
  - Interface with user-space programs.

- Designed and implemented a user-space application to format a block device (simulated by a file) into the format of the custom file system.

- Designed a user-space test case application to validate the file system's open/read/write/ls/cd operations.

This file system is suitable for data-intensive application scenarios and includes the following features:

1. **Lazy Initialization with Block Groups**: Improves large storage management; only the first block group is initialized at mkfs, and additional block groups are initialized as needed when the first block group is used up.
2. **Circular Block Group Allocation**: Each block group has its own inode_bitmap and block_bitmap, making management more efficient compared to traditional file systems.
3. **Performance Near Linux's Built-in File System**: In data-intensive applications, a directory's inode often indexes multiple directory entries, which can reduce performance when searching for directory entries. Traditional file systems use serial traversal to search for directory entries; we use directory hashing, constructing a **hashed B+ tree** in the inode when the number of directory entries exceeds a certain threshold, significantly improving performance.
4. **Inode Indexing for Large Files**: Inspired by ext4, we use extent trees to avoid performance degradation when indexing large files. B+ trees enhance the efficiency of large file operations (e.g., lookups).
5. **Built-in Permission Management**: On top of the basic Linux permission model, we have implemented stricter permission management, such as prohibiting a user from deleting or renaming another user's files/directories, while allowing file/directory creation in the root directory (`/`) for all users. Users can only create subdirectories/files in their own directory. The kernel displays related permission management prompts.
6. **Performance Comparison**: We have designed a `test` structure and a `makefile` file. Using the `make test` command, you can test the performance of all files in the test folder. Use `make test k` to test the k-th file (starting from 1).

The file system supports the following commands:

```shell
mkdir -p dir1  # Create directory, calls XCraft_mkdir in inode.c
touch 1.txt    # Create file, calls XCraft_create in inode.c
rm 1.txt       # Delete file, calls XCraft_unlink in inode.c
rm -d dir1     # Delete directory, calls XCraft_unlink in inode.c
echo "hello" > 1.txt  # Write to file, calls XCraft_file_writer_iter
cat 1.txt      # Read file content, calls XCraft_file_read_iter
ls             # Show directory entries, calls XCraft_readdir
mv /mnt/test/dir1/1.txt /mnt/test/dir2/  # Move file, calls XCraft_rename
```