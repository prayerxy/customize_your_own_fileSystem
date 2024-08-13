#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_FILES 1000
#define PATH_PREFIX "/mnt/test/"

//延迟初始化（块组）  测试
/*
延迟初始化的块组可以减少初始创建时的开销。
以下程序测试大量文件和目录的创建和删除，以观察延迟初始化的效果。
*/
void create_and_delete_files() {
    char path[256];
    char content[100] = "Hello World!";
    FILE *fp;
    for (int i = 0; i < NUM_FILES; i++) {
        sprintf(path, "%sfile%d.txt", PATH_PREFIX, i);
        fp = fopen(path, "w");
        if (fp == NULL) {
            perror("创建文件失败");
            exit(EXIT_FAILURE);
        }
        fwrite(content, strlen(content), 1, fp);
        fclose(fp);
    }
    
    printf("已创建 %d 个文件\n", NUM_FILES);

    for (int i = 0; i < NUM_FILES; i++) {
        sprintf(path, "%sfile%d.txt", PATH_PREFIX, i);
        if (remove(path) == -1) {
            perror("删除文件失败");
            exit(EXIT_FAILURE);
        }
    }

    printf("已删除 %d 个文件\n", NUM_FILES);
}

int main() {
    printf("##########test 4##########\n");
    clock_t start = clock();
    create_and_delete_files();
    printf("延迟初始化测试耗时: %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    printf("##########test 4##########\n");
    return 0;
}
