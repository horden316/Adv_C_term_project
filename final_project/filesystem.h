#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 100
#define MAX_FILENAME 255
#define BLOCK_SIZE 1024
#define FS_IMG "filesystem.img"

// 定義 File 結構
typedef struct {
    char name[MAX_FILENAME]; // 檔案或目錄名稱
    int size;                // 檔案大小（目錄則為 0）
    int start_block;         // 起始區塊
    int used_blocks;         // 使用的區塊數
    int is_directory;        // 是否為目錄（1 表示目錄，0 表示檔案）
} File;

// 定義 FileSystem 結構
typedef struct {
    char current_path[MAX_FILENAME]; // 當前目錄路徑
    int partition_size;              // 分區大小
    int total_blocks;                // 總區塊數
    int free_blocks;                 // 剩餘區塊數
    File files[MAX_FILES];           // 檔案和目錄清單
    char storage[BLOCK_SIZE * MAX_FILES]; // 模擬存儲區域
} FileSystem;

// 初始化檔案系統
void init_filesystem(FileSystem *fs, int size);

// 載入檔案系統
void load_filesystem(FileSystem *fs, const char *filename);

// 儲存檔案系統
void save_filesystem(FileSystem *fs, const char *filename);

// 列出目錄內容
void ls(FileSystem *fs);

// 建立目錄
void mkdir(FileSystem *fs, const char *dirname);

// 刪除目錄
void rmdir(FileSystem *fs, const char *dirname);

// 切換目錄
void cd(FileSystem *fs, const char *path);

// 將檔案存入檔案系統
void put(FileSystem *fs, const char *filename);

// 從檔案系統取出檔案
void get(FileSystem *fs, const char *filename);

// 刪除檔案
void rm(FileSystem *fs, const char *filename);

// 顯示檔案內容
void cat(FileSystem *fs, const char *filename);

// 顯示檔案系統狀態
void status(FileSystem *fs);

// 列出可用指令
void help();

// 儲存並退出檔案系統
void exit_and_store(FileSystem *fs);

#endif

