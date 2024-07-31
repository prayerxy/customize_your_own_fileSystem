#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define NUM_DIRS 10000
#define FILES_PER_DIR 1
#define PATH_PREFIX "/mnt/test/"

/*
目录哈希能有效避免在一个目录下遍历大量子目录。
以下程序在一个目录下创建大量子目录，然后在这些子目录内创建文件，并执行读写操作。
*/

void create_dirs_and_files() {
    char dir_path[256];
    char file_path[256];
    char content[100] = "Hello World! we are praye4u.";
    FILE *fp;


    for (int i = 0; i < NUM_DIRS; i++) {
        sprintf(dir_path, "%sdir%d", PATH_PREFIX, i);
        if (mkdir(dir_path, 0777) == -1) {
            perror("创建目录失败");
            exit(EXIT_FAILURE);
        }
        
        for (int j = 0; j < FILES_PER_DIR; j++) {
            sprintf(file_path, "%s/file%d.txt", dir_path, j);
            fp = fopen(file_path, "w");
            if (fp == NULL) {
                perror("创建文件失败");
                exit(EXIT_FAILURE);
            }
            fwrite(content, strlen(content), 1, fp);
            fclose(fp);
        }
    }

    printf("已创建 %d 个目录，每个目录 %d 个文件\n", NUM_DIRS, FILES_PER_DIR);
}

void read_files_in_dirs() {
    char file_path[256];
    char buffer[100];

    for (int i = 0; i < NUM_DIRS; i++) {
        for (int j = 0; j < FILES_PER_DIR; j++) {
            sprintf(file_path, "%sdir%d/file%d.txt", PATH_PREFIX, i, j);
            FILE *fp = fopen(file_path, "r");
            if (fp == NULL) {
                perror("读取文件失败");
                exit(EXIT_FAILURE);
            }
            while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                printf("%s", buffer);
            }
            fclose(fp);
        }
    }

    printf("\n已读取所有文件\n");
}

void delete_files_and_dirs() {
    char file_path[256];
    char dir_path[256];

    for (int i = 0; i < NUM_DIRS; i++) {
        for (int j = 0; j < FILES_PER_DIR; j++) {
            sprintf(file_path, "%sdir%d/file%d.txt", PATH_PREFIX, i, j);
            if (remove(file_path) == -1) {
                perror("删除文件失败");
                exit(EXIT_FAILURE);
            }
        }
        sprintf(dir_path, "%sdir%d", PATH_PREFIX, i);
        if (rmdir(dir_path) == -1) {
            perror("删除目录失败");
            exit(EXIT_FAILURE);
        }
    }

    printf("已删除所有文件和目录\n");
}

int main() {
    clock_t start = clock();
    create_dirs_and_files();
    read_files_in_dirs();
    delete_files_and_dirs();
    printf("目录哈希测试耗时: %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    return 0;
}
