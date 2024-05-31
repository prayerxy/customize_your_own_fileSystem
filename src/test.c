#include <stdint.h>
#include<stdio.h>
#include<sys/stat.h>
#include<endian.h>
#include "../include/XCraft.h"
struct temp{
    int a;
    int b;
    int c;
    char data[7];
};
int main(){
    struct temp fuck[2],*r=fuck+1;
    fuck[1].a=1;
    fuck[1].b=2;
    fuck[1].c=3;
    fuck[1].data[0]=1;
    fuck[1].data[1]=2;
    
    printf("%ld",r->c);
    printf("%ld",r-fuck);
    return 0;
}