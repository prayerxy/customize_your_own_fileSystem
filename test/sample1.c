//测试文件系统性能
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main(){
    //文件系统路径  nku.txt不存在时会自动创建？
    printf("###########test 1#########\n");
    char path[100] = "/mnt/test/nku.txt";
    //文件内容
    char content[100] = "Hello World!";
    //文件指针
    FILE *fp;
    //文件读写测试
    fp = fopen(path, "w");
    if(fp == NULL){
        printf("open file failed\n");
        return -1;
    }
    //使用循环测试文件写性能
    clock_t start = clock();
    for(int i = 0; i < 10000000; i++){
        fwrite(content, strlen(content), 1, fp);
    }
    clock_t end = clock();
    //文件读写测试
    fp = fopen(path, "r");
    if(fp == NULL){
        printf("open file failed\n");
        return -1;
    }
    char buf[100];
    fread(buf, strlen(content), 1, fp);
    printf("file content: %s\n", buf);
    fclose(fp);
    //文件删除测试
    if(remove(path) == 0){
        printf("file deleted\n");
    }else{
        printf("delete file failed\n");
    }
    //输出文件读写性能
    printf("file write performance: %f\n", (double)(end - start) / CLOCKS_PER_SEC);
    printf("##########test 1##########\n");
    return 0;
}
