#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

void create_dir(const char *path) {
    if (mkdir(path, 0777) == -1) {
        perror("创建目录失败");
        exit(EXIT_FAILURE);
    }
}

void create_file(const char *path) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("创建文件失败");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

void delete_file(const char *path) {
    if (remove(path) == -1) {
        perror("删除文件失败");
        exit(EXIT_FAILURE);
    }
}

void delete_dir(const char *path) {
    if (rmdir(path) == -1) {
        perror("删除目录失败");
        exit(EXIT_FAILURE);
    }
}

void write_to_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("写入文件失败");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%s", content);
    fclose(fp);
}

void read_file(const char *path) {
    char buffer[256];
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("读取文件失败");
        exit(EXIT_FAILURE);
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s", buffer);
    }
    fclose(fp);
}

void list_dir(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);
    if (dp == NULL) {
        perror("打开目录失败");
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dp))) {
        printf("%s\n", entry->d_name);
    }
    closedir(dp);
}

void move_file(const char *src, const char *dest) {
    if (rename(src, dest) == -1) {
        perror("移动文件失败");
        exit(EXIT_FAILURE);
    }
}

int main() {
    char *dir1 = "/mnt/test/dir1";
    char *dir2 = "/mnt/test/dir2";
    char *file1 = "/mnt/test/dir1/1.txt";
    char *new_file1 = "/mnt/test/dir2/1.txt";
    char *content = "hello";
    printf("##########test 3##########\n");
    // 创建目录
    create_dir(dir1);
    printf("创建目录: %s\n", dir1);

    // 创建文件
    create_file(file1);
    printf("创建文件: %s\n", file1);

    // 写入文件内容
    write_to_file(file1, content);
    printf("写入文件内容: %s\n", content);

    // 读取文件内容
    printf("读取文件内容:\n");
    read_file(file1);

    // 显示目录项
    printf("目录内容:\n");
    list_dir(dir1);

    // // 剪切文件
    create_dir(dir2);  // 先创建目标目录
    move_file(file1, new_file1);
    printf("文件剪切至: %s\n", new_file1);

    // 删除文件
    delete_file(new_file1);
    printf("删除文件: %s\n", new_file1);

    // 删除目录
    delete_dir(dir1);
    delete_dir(dir2);
    printf("删除目录: %s 和 %s\n", dir1, dir2);
    printf("##########test 3##########\n");
    return 0;
}
