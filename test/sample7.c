#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FILE_PATH "/mnt/test/large_file.txt"
#define CHUNK_SIZE 1024 * 1024  // 1MB
#define NUM_CHUNKS 180  // 总共 100MB
#define SMALL_CHUNK_SIZE 100  // 512KB

void write_large_file() {
    char content[SMALL_CHUNK_SIZE];
    memset(content, 'A', SMALL_CHUNK_SIZE);
    FILE *fp = fopen(FILE_PATH, "w");
    if (fp == NULL) {
        perror("创建大文件失败");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < NUM_CHUNKS; i++) {
        for (int j = 0; j < CHUNK_SIZE / SMALL_CHUNK_SIZE; j++) {
            size_t written = fwrite(content, 1, SMALL_CHUNK_SIZE, fp);
            if (written != SMALL_CHUNK_SIZE) {
                perror("写入文件失败");
                fclose(fp);
                exit(EXIT_FAILURE);
            }
        }
    }
    fflush(fp);
    fclose(fp);
    printf("已写入大文件 %d MB\n", NUM_CHUNKS);
}

void read_large_file() {
    char buffer[CHUNK_SIZE];
    FILE *fp = fopen(FILE_PATH, "r");
    if (fp == NULL) {
        perror("读取大文件失败");
        exit(EXIT_FAILURE);
    }
    size_t read_size;
    while ((read_size = fread(buffer, 1, CHUNK_SIZE, fp)) == CHUNK_SIZE) {
        // 读取内容
    }
    fclose(fp);
    printf("已读取大文件 %d MB\n", NUM_CHUNKS);
}

int main() {
    clock_t start = clock();
    write_large_file();
    read_large_file();
    printf("大文件扩展树测试耗时: %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    return 0;
}
