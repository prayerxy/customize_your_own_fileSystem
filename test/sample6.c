#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
文件系统权限管理测试
以下程序测试文件系统的权限管理功能。

事先系统存在两个用户 user1 和 user2，分别创建文件 /mnt/test/user1_testfile.txt 和 /mnt/test/user2_testfile.txt。

内容：每一个用户只能读写自己的文件，但是可以读别人的文件  不能写别人的文件
root可以写任何/读任何文件
*/

// 在子进程中执行命令
int execute_command_as_user(const char *username, const char *password, const char *command) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // 子进程：执行su命令
        execlp("su", "su", "-", username, "-c", command, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // 父进程：等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return 0;
        } else {
            return -1;
        }
    }
}

int main() {
    // 用户名和密码
    const char *user1 = "user1";
    const char *user2 = "user2";
    const char *password1 = "Password1234";
    const char *password2 = "Password1234";

    // 文件路径
    const char *user1_file = "/mnt/test/user1_testfile.txt";
    const char *user2_file = "/mnt/test/user2_testfile.txt";
    printf("##########test 6##########\n");
    // 用户1创建文件
    if (execute_command_as_user(user1, password1, "touch /mnt/test/user1_testfile.txt") == 0) {
        printf("User1: Successfully created file in /mnt/test/\n");
    } else {
        printf("User1: Failed to create file in /mnt/test/\n");
    }

    // 用户2创建文件
    if (execute_command_as_user(user2, password2, "touch /mnt/test/user2_testfile.txt") == 0) {
        printf("User2: Successfully created file in /mnt/test/\n");
    } else {
        printf("User2: Failed to create file in /mnt/test/\n");
    }

    // 用户1尝试重命名用户2的文件
    if (execute_command_as_user(user1, password1, "mv /mnt/test/user2_testfile.txt /mnt/test/user2_testfile_renamed.txt") == 0) {
        printf("User1: Successfully renamed user2's file (unexpected)\n");
    } else {
        printf("User1: Failed to rename user2's file (expected)\n");
    }

    // 用户1尝试删除用户2的文件
    if (execute_command_as_user(user1, password1, "rm /mnt/test/user2_testfile.txt") == 0) {
        printf("User1: Successfully deleted user2's file (unexpected)\n");
    } else {
        printf("User1: Failed to delete user2's file (expected)\n");
    }

    // 用户2尝试读取用户1的文件
    if (execute_command_as_user(user2, password2, "cat /mnt/test/user1_testfile.txt") == 0) {
        printf("User2: Successfully read user1's file (expected)\n");
    } else {
        printf("User2: Failed to read user1's file (unexpected)\n");
    }

    // 用户2尝试写入用户1的文件
    if (execute_command_as_user(user2, password2, "echo 'User2 wrote this.' > /mnt/test/user1_testfile.txt") == 0) {
        printf("User2: Successfully wrote to user1's file (unexpected)\n");
    } else {
        printf("User2: Failed to write to user1's file (expected)\n");
    }
    printf("##########test 6##########\n");
    return 0;
}
