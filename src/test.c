
#include<stdio.h>
#include<sys/stat.h>
#include<endian.h>
#include "../include/XCraft.h"
int main(){

    printf("%ld",htole16(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR |
                            S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH));
    return 0;
}