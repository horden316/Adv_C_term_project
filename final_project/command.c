#include "command.h"

void ls(FileSystem *fs) {
    printf("Current Path: '%s'\n", fs->current_path);
    printf("Files:\n");
    for (int i = 0; i < fs->file_count; i++) {
        printf("Debug: File '%s', Parent: '%s', Directory: %d\n",
               fs->files[i].name,
               fs->files[i].parent_name,
               fs->files[i].is_directory);

        // 嚴格比對 parent_name 和 current_path
        if (fs->files[i].used_blocks > 0 && 
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            
            if (fs->files[i].is_directory) {
                printf("[DIR] %s\n", fs->files[i].name);
            } else {
                printf("%s (%d bytes)\n", fs->files[i].name, fs->files[i].size);
            }
        }
    }
}

void mkdir(FileSystem *fs, const char *dirname) {
    // 檢查目錄是否已存在
    for (int i = 0; i < fs->file_count; i++) {
        if (fs->files[i].used_blocks > 0 && 
            strcmp(fs->files[i].name, dirname) == 0 && 
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            printf("Error: Directory '%s' already exists.\n", dirname);
            return;
        }
    }

    // 檢查是否有足夠的空間來創建新目錄
    if (fs->free_blocks < 1) {
        printf("Error: Not enough space to create directory '%s'.\n", dirname);
        return;
    }

    // 動態分配新的文件結構
    fs->files = (File *)realloc(fs->files, (fs->file_count + 1) * sizeof(File));
    if (fs->files == NULL) {
        printf("Error: Could not allocate memory for new directory.\n");
        return;
    }

    // 初始化新目錄
    File *new_dir = &fs->files[fs->file_count];
    strncpy(new_dir->name, dirname, MAX_FILENAME);
    new_dir->size = 0;
    new_dir->used_blocks = 1; // 目錄至少佔用一個區塊
    new_dir->is_directory = 1; // 標記為目錄
    snprintf(new_dir->parent_name, MAX_FILENAME, "%s", fs->current_path); // 確保正確設定父目錄名稱
    fs->file_count++;
    fs->free_blocks--;

    printf("Directory '%s' created.\n", dirname);
}


void rmdir(FileSystem *fs, const char *dirname) {
    for (int i = 0; i < fs->file_count; i++) {
        if (fs->files[i].used_blocks > 0 && 
            fs->files[i].is_directory && 
            strcmp(fs->files[i].name, dirname) == 0 && 
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            
            // 組合完整路徑
            char full_path[MAX_PATH];
            if (strcmp(fs->current_path, "/") == 0) {
                snprintf(full_path, MAX_PATH, "/%s", dirname);
            } else {
                snprintf(full_path, MAX_PATH, "%s/%s", fs->current_path, dirname);
            }

            // 檢查此目錄有沒有 children
            for (int j = 0; j < fs->file_count; j++) {
                if (fs->files[j].used_blocks > 0 && 
                    strcmp(full_path, fs->files[j].parent_name) == 0) {
                    printf("Error: Directory '%s' is not empty.\n", dirname);
                    return;
                }
            }

            // 清空目錄屬性
            memset(&fs->files[i], 0, sizeof(File));
            fs->free_blocks++;
            printf("Directory '%s' removed.\n", dirname);

            // 如果是最後一個檔案，減少 `file_count`
            if (i == fs->file_count - 1) {
                fs->file_count--;
            }
            return;
        }
    }

    printf("Error: Directory '%s' not found in current directory.\n", dirname);
}



void cd(FileSystem *fs, const char *path) {
    if (strcmp(path, "..") == 0) { 
        char *last_slash = strrchr(fs->current_path, '/');
        if (last_slash && last_slash != fs->current_path) {
            *last_slash = '\0';
        } else {
            strcpy(fs->current_path, "/");
        }
    } else {
        for (int i = 0; i < fs->file_count; i++) {
            if (fs->files[i].used_blocks > 0 &&
                fs->files[i].is_directory &&
                strcmp(fs->files[i].name, path) == 0 &&
                strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
                
                if (strcmp(fs->current_path, "/") == 0) {
                    snprintf(fs->current_path, MAX_PATH, "/%s", path);
                } else {
                    snprintf(fs->current_path, MAX_PATH, "%s/%s", fs->current_path, path);
                }
                
                printf("Current directory: %s\n", fs->current_path);
                return;
            }
        }
        printf("Error: Directory '%s' not found.\n", path);
    }
}


void put(FileSystem *fs, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file '%s'.\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    int filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    int required_blocks = (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (required_blocks > fs->free_blocks) {
        printf("Error: Not enough space to store file '%s'.\n", filename);
        fclose(file);
        return;
    }

    fs->files = (File *)realloc(fs->files, (fs->file_count + 1) * sizeof(File));
    if (fs->files == NULL) {
        printf("Error: Could not allocate memory for new file.\n");
        fclose(file);
        return;
    }

    File *new_file = &fs->files[fs->file_count];
    strncpy(new_file->name, filename, MAX_FILENAME);
    strncpy(new_file->parent_name, fs->current_path, MAX_FILENAME); // 設定父目錄
    new_file->size = filesize;
    new_file->used_blocks = required_blocks;
    new_file->is_directory = 0; // 標記為檔案
    new_file->start_block = fs->storage_start_block + fs->file_count * BLOCK_SIZE;

    fread(storage + new_file->start_block, filesize, 1, file);
    fs->free_blocks -= required_blocks;
    fs->file_count++;
    fclose(file);
    printf("File '%s' added to filesystem.\n", filename);
}


void get(FileSystem *fs, const char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0) {
            FILE *file = fopen(filename, "w");
            fwrite(storage + (i * BLOCK_SIZE), fs->files[i].size, 1, file);
            fclose(file);
            printf("File '%s' retrieved from filesystem.\n", filename);
            return;
        }
    }
    printf("Error: File '%s' not found.\n", filename);
}

void rm(FileSystem *fs, const char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0) {
            fs->free_blocks += fs->files[i].used_blocks;
            memset(&fs->files[i], 0, sizeof(File));
            printf("File '%s' removed from filesystem.\n", filename);
            return;
        }
    }
    printf("Error: File '%s' not found.\n", filename);
}

void cat(FileSystem *fs, const char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0) {
            printf("Contents of file '%s':\n%s\n", filename, storage + (i * BLOCK_SIZE));
            return;
        }
    }
    printf("Error: File '%s' not found.\n", filename);
}

void status(FileSystem *fs) {
    int used_inodes = 0, used_blocks = 0, file_blocks = 0;
    // for (int i = 0; i < fs->file_count i++) {
    //     if (fs->files[i].used_blocks > 0) {
    //         used_inodes++;
    //         used_blocks += fs->files[i].used_blocks;
    //         file_blocks += (fs->files[i].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    //     }
    // }

    printf("partition size: %d\n", fs->partition_size);
    //printf("total inodes: %d\n", );
    printf("used inodes: %d\n", used_inodes);
    printf("total blocks: %d\n", fs->total_blocks);
    printf("used blocks: %d\n", used_blocks);
    printf("files' blocks: %d\n", file_blocks);
    printf("block size: %d\n", BLOCK_SIZE);
    printf("free space: %d\n", fs->partition_size - (used_blocks * BLOCK_SIZE));
}


void help() {
    printf("List of commands:\n");
    printf("'ls'    list directory\n");
    printf("'cd'    change directory\n");
    printf("'rm'    remove\n");
    printf("'mkdir' make directory\n");
    printf("'rmdir' remove directory\n");
    printf("'put'   put file into the space\n");
    printf("'get'   get file from the space\n");
    printf("'cat'   show content\n");
    printf("'status' show status of the space\n");
    printf("'help'  list commands\n");
    printf("'exit and store img' exit and save filesystem\n");


}