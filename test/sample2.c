//测试文件系统性能
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main(){
    // "/mnt/test"下面文件系统测试，创建10000个目录再读写，删除
    printf("##########test 2##########\n");
    char path[100] = "/mnt/test/";
    char content[100] = "Hello World!";
    FILE *fp;
    clock_t start = clock();

    for(int i = 0; i < 10000; i++){
        char dir[100];
        //不按照i去创建目录名，随机一个目录名
        sprintf(dir, "%s%d", path, i);
        if(mkdir(dir, 0777) == -1){
            printf("create dir failed\n");
            return -1;
        }
        char file[100];
        sprintf(file, "%s%d/nku.txt", path, i);
        fp = fopen(file, "w");
        if(fp == NULL){
            printf("open file failed\n");
            return -1;
        }
        fwrite(content, strlen(content), 1, fp);
        fclose(fp);
    }
    printf("alread write 10000 files\n");
    //删除目录
    for(int i = 0; i < 10000; i++){
        //删除文件
        char file[100];
        sprintf(file, "%s%d/nku.txt", path, i);
        if(remove(file) == -1){
            printf("delete file failed\n");
        }
        //删除目录
        char dir[100];
        sprintf(dir, "%s%d", path, i);
        if(rmdir(dir) == -1){
            printf("delete dir failed\n");
            return -1;
        }
    }
    printf("alread delete 10000 files\n");
    printf("file write performance: %f\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    printf("#########test 2###########\n");
}
