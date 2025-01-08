#include "main.h"

int main() {
    FileSystem fs;
    char command[256], arg1[256];
    int running = 1;

    printf("1. Load from file\n2. Create new partition\n");
    int option;
    scanf("%d", &option);
    getchar();

    if (option == 1) {
        printf("Please input the filename of the filesystem image: ");
        char filename[256];
        scanf("%s", filename);
        getchar();
        load_filesystem(&fs, filename);
    } else if (option == 2) {
        printf("Input size of a new partition (example 102400): ");
        int size;
        scanf("%d", &size);
        getchar();
        printf("partition size = %d\n", size);
        init_filesystem(&fs, size, 0);//第一個分區從0開始
        printf("Make new partition successful!\n");
        help();
    }

     while (running) {
        printf("%s$ ", fs.current_path); // 顯示當前目錄
        scanf("%s", command);

        if (strcmp(command, "ls") == 0) {
            ls(&fs);
        } else if (strcmp(command, "mkdir") == 0) {
            scanf("%s", arg1);
            mkdir(&fs, arg1);
        } else if (strcmp(command, "rmdir") == 0) {
            scanf("%s", arg1);
            rmdir(&fs, arg1);
        } else if (strcmp(command, "put") == 0) {
            scanf("%s", arg1);
            put(&fs, arg1);
        } else if (strcmp(command, "get") == 0) {
            scanf("%s", arg1);
            get(&fs, arg1);
        } else if (strcmp(command, "cat") == 0) {
            scanf("%s", arg1);
            cat(&fs, arg1);
        } else if (strcmp(command, "rm") == 0) {
            scanf("%s", arg1);
            rm(&fs, arg1);
        } else if (strcmp(command, "status") == 0) {
            status(&fs);
        } else if (strcmp(command, "help") == 0) {
            help();
        } else if (strcmp(command, "cd") == 0) {
            scanf("%s", arg1);
            cd(&fs, arg1);
        } else if (strcmp(command, "exit") == 0) {
            exit_and_store(&fs);
            running = 0;
        } else {
            printf("Unknown command: '%s'. Type 'help' for a list of commands.\n", command);
        }
    }

    return 0;
}
