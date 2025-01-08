#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 255
#define MAX_PATH 1023
#define BLOCK_SIZE 1024

// 定義 File 結構
typedef struct File {
    char name[MAX_FILENAME]; // 檔案或目錄名稱
    int size;                // 檔案大小（目錄則為 0）
    int start_block;         // 起始區塊
    int used_blocks;         // 使用的區塊數
    int is_directory;        // 是否為目錄（1 表示目錄，0 表示檔案）
    char parent_name[MAX_FILENAME]; // 父目錄名稱
} File;

// 定義 FileSystem 結構
typedef struct {
    char current_path[MAX_PATH]; // 目前目錄路徑
    int partition_size;              // 分區大小
    int total_blocks;                // 總區塊數
    int free_blocks;                 // 剩餘區塊數
    int storage_start_block;         // 在共享存儲區域中的起始區塊（如果一個storage裡面有多個FileSystem的話啦）
    File *files;                     // 動態分配的檔案和目錄清單
    int file_count;                  // 檔案和目錄的數量
    char *used_blocks_bitmask;       // 已使用空間的bitmask
} FileSystem;

// 共享儲存storage
extern char storage[];

// 初始化檔案系統
void init_filesystem(FileSystem *fs, int size, int start_block);

// 載入檔案系統
void load_filesystem(FileSystem *fs);

// 儲存檔案系統
void save_filesystem(FileSystem *fs, const char *filename);

//從storage_used_blocks裡找出連續可用的區塊
int find_free_blocks(FileSystem *fs, int required_blocks);

// 設定bitmask
void set_bitmask(FileSystem *fs, int start_block, int required_blocks);

// 清除bitmask
void clear_bitmask(FileSystem *fs, int start_block, int required_blocks);

// 印出bitmask
void print_bitmask(FileSystem *fs);

// 儲存並退出檔案系統
void exit_and_store(FileSystem *fs);

#endif

