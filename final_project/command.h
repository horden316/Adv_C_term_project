#ifndef COMMAND_H
#define COMMAND_H
#include "filesystem.h"

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

void create(FileSystem *fs, const char *filename) ;
void edit(FileSystem *fs, const char *filename) ;
// 列出可用指令
void help();

#endif