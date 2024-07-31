#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

void create_dir(const char *path) {
    if (mkdir(path, 0777) == -1) {
        perror("创建目录失败");
        exit(EXIT_FAILURE);
    }
}

void create_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("创建文件失败");
        exit(EXIT_FAILURE);
    }
    fwrite(content, strlen(content), 1, fp);
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

void write_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "a");
    if (fp == NULL) {
        perror("打开文件失败");
        exit(EXIT_FAILURE);
    }
    fwrite(content, strlen(content), 1, fp);
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
    char path[100] = "/mnt/test/";
    char content[100] = "Hello World!";
    char new_content[100] = "Hello, this is a new line!\n";
    printf("##########test 3##########\n");
    clock_t start = clock();

    // 创建目录和文件
    for (int i = 0; i < 10; i++) {
        char dir[100];
        char subdir[100];
        char file[100];

        sprintf(dir, "%sdir%d", path, i);
        create_dir(dir);

        sprintf(subdir, "%s/subdir", dir);
        create_dir(subdir);

        sprintf(file, "%s/file%d.txt", subdir, i);
        create_file(file, content);
    }

    printf("已创建10个目录及文件\n");

    // 写入文件内容
    for (int i = 0; i < 10; i++) {
        char file[100];
        sprintf(file, "%sdir%d/subdir/file%d.txt", path, i, i);
        write_file(file, new_content);
    }

    printf("已写入文件内容\n");

    // 读取文件内容
    for (int i = 0; i < 10; i++) {
        char file[100];
        sprintf(file, "%sdir%d/subdir/file%d.txt", path, i, i);
        read_file(file);
    }

    printf("已读取文件内容\n");

    // 显示目录项
    for (int i = 0; i < 10; i++) {
        char dir[100];
        sprintf(dir, "%sdir%d/subdir", path, i);
        printf("目录 %s:\n", dir);
        list_dir(dir);
    }

    // 剪切文件
    for (int i = 0; i < 10; i++) {
        char src[100];
        char dest[100];
        sprintf(src, "%sdir%d/subdir/file%d.txt", path, i, i);
        sprintf(dest, "%sdir%d/file%d.txt", path, i, i);
        move_file(src, dest);
    }

    printf("已剪切文件\n");

    // 删除文件和目录
    for (int i = 0; i < 10; i++) {
        char file[100];
        char subdir[100];
        char dir[100];

        sprintf(file, "%sdir%d/file%d.txt", path, i, i);
        delete_file(file);

        sprintf(subdir, "%sdir%d/subdir", path, i);
        delete_dir(subdir);

        sprintf(dir, "%sdir%d", path, i);
        delete_dir(dir);
    }

    printf("已删除文件和目录\n");

    printf("文件系统测试总耗时: %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    printf("##########test 3##########\n");
    return 0;
}
