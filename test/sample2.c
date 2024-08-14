//测试文件系统性能
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define DIR_NUM 15000
int main(){
    // "/mnt/test"下面文件系统测试，创建15000个目录再删除
    printf("##########test 2##########\n");
    char path[100] = "/mnt/test/";
    char content[100] = "Hello World!";
    FILE *fp;
    clock_t start = clock();

    for(int i = 0; i < DIR_NUM; i++){
        char dir[100];
        //不按照i去创建目录名，随机一个目录名
        sprintf(dir, "%s%d", path, i);
        if(mkdir(dir, 0777) == -1){
            printf("create dir failed\n");
            return -1;
        }
       
    }
    printf("alread write %d dirs\n",DIR_NUM);
    //删除目录
    for(int i = 0; i < DIR_NUM; i++){
        //删除目录
        char dir[100];
        sprintf(dir, "%s%d", path, i);
        if(rmdir(dir) == -1){
            printf("delete dir failed\n");
            return -1;
        }
    }
    printf("alread delete %d dirs\n",DIR_NUM);
    printf("file write performance: %f\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    printf("#########test 2###########\n");
}

