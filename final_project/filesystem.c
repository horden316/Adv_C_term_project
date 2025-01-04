#include "filesystem.h"

void init_filesystem(FileSystem *fs, int size) {
    fs->partition_size = size;
    fs->total_blocks = size / BLOCK_SIZE;
    fs->free_blocks = fs->total_blocks;
    memset(fs->files, 0, sizeof(fs->files));
    memset(fs->storage, 0, sizeof(fs->storage));
}

void load_filesystem(FileSystem *fs, const char *filename) {
    // 確保檔案名稱以 ".img" 結尾
    if (!strstr(filename, ".img")) {
        printf("Error: The file '%s' is not a valid .img dump file.\n", filename);
        return;
    }

    FILE *file = fopen(filename, "rb");
    if (file) {
        fread(fs, sizeof(FileSystem), 1, file);
        fclose(file);
        printf("Filesystem loaded from '%s'.\n", filename);
    } else {
        printf("Error: Could not load filesystem. Creating new filesystem.\n");
        init_filesystem(fs, BLOCK_SIZE * MAX_FILES);
    }
}

void save_filesystem(FileSystem *fs, const char *filename) {
    // 確保檔案名稱以 ".img" 結尾
    if (!strstr(filename, ".img")) {
        printf("Error: The file '%s' is not a valid .img dump file.\n", filename);
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(fs, sizeof(FileSystem), 1, file);
        fclose(file);
        printf("Filesystem saved to '%s'.\n", filename);
    } else {
        printf("Error: Could not save filesystem.\n");
    }
}


void ls(FileSystem *fs) {
    printf("Files:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used_blocks > 0) {
            if (fs->files[i].is_directory) {
                printf("[DIR] %s\n", fs->files[i].name);
            } else {
                printf("%s (%d bytes)\n", fs->files[i].name, fs->files[i].size);
            }
        }
    }
}


void mkdir(FileSystem *fs, const char *dirname) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used_blocks > 0 && strcmp(fs->files[i].name, dirname) == 0) {
            printf("Error: Directory '%s' already exists.\n", dirname);
            return;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used_blocks == 0) {
            strncpy(fs->files[i].name, dirname, MAX_FILENAME);
            fs->files[i].size = 0;
            fs->files[i].used_blocks = 1; // 目錄至少占用一個 inode
            fs->files[i].is_directory = 1; // 標記為目錄
            printf("Directory '%s' created.\n", dirname);
            return;
        }
    }

    printf("Error: No space left to create directory.\n");
}
void rmdir(FileSystem *fs, const char *dirname) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used_blocks > 0 && strcmp(fs->files[i].name, dirname) == 0) {
            if (!fs->files[i].is_directory) {
                printf("Error: '%s' is not a directory.\n", dirname);
                return;
            }

            // 確認目錄是否為空
            for (int j = 0; j < MAX_FILES; j++) {
                if (fs->files[j].used_blocks > 0 && strstr(fs->files[j].name, dirname) == fs->files[j].name) {
                    printf("Error: Directory '%s' is not empty.\n", dirname);
                    return;
                }
            }

            // 刪除目錄
            fs->files[i].used_blocks = 0;
            memset(&fs->files[i], 0, sizeof(File));
            printf("Directory '%s' removed.\n", dirname);
            return;
        }
    }

    printf("Error: Directory '%s' not found.\n", dirname);
}

void cd(FileSystem *fs, const char *path) {
    if (strcmp(path, "..") == 0) {
        // 返回上一層目錄
        char *last_slash = strrchr(fs->current_path, '/');
        if (last_slash && last_slash != fs->current_path) {
            *last_slash = '\0';
        } else {
            strcpy(fs->current_path, "/");
        }
    } else {
        // 檢查目錄是否存在
        for (int i = 0; i < MAX_FILES; i++) {
            if (fs->files[i].used_blocks > 0 && 
                fs->files[i].is_directory && 
                strcmp(fs->files[i].name, path) == 0) {

                // 切換到指定目錄
                if (strcmp(fs->current_path, "/") == 0) {
                    snprintf(fs->current_path, MAX_FILENAME, "/%s", path);
                } else {
                    snprintf(fs->current_path, MAX_FILENAME, "%s/%s", fs->current_path, path);
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

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used_blocks == 0) {
            strncpy(fs->files[i].name, filename, MAX_FILENAME);
            fs->files[i].size = filesize;
            fs->files[i].used_blocks = (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;

            fread(fs->storage + (i * BLOCK_SIZE), filesize, 1, file);
            fs->free_blocks -= fs->files[i].used_blocks;
            fclose(file);
            printf("File '%s' added to filesystem.\n", filename);
            return;
        }
    }
    fclose(file);
    printf("Error: No space available to store file.\n");
}

void get(FileSystem *fs, const char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fs->files[i].name, filename) == 0) {
            FILE *file = fopen(filename, "w");
            fwrite(fs->storage + (i * BLOCK_SIZE), fs->files[i].size, 1, file);
            fclose(file);
            printf("File '%s' retrieved from filesystem.\n", filename);
            return;
        }
    }
    printf("Error: File '%s' not found.\n", filename);
}

void rm(FileSystem *fs, const char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
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
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fs->files[i].name, filename) == 0) {
            printf("Contents of file '%s':\n%s\n", filename, fs->storage + (i * BLOCK_SIZE));
            return;
        }
    }
    printf("Error: File '%s' not found.\n", filename);
}

void status(FileSystem *fs) {
    int used_inodes = 0, used_blocks = 0, file_blocks = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used_blocks > 0) {
            used_inodes++;
            used_blocks += fs->files[i].used_blocks;
            file_blocks += (fs->files[i].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        }
    }

    printf("partition size: %d\n", fs->partition_size);
    printf("total inodes: %d\n", MAX_FILES);
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

void exit_and_store(FileSystem *fs) {
    char filename[MAX_FILENAME];

    printf("Enter the filename to save the filesystem (e.g., my_filesystem.img): ");
    scanf("%s", filename);

    // 檢查檔案名稱是否以 ".img" 結尾
    if (!strstr(filename, ".img")) {
        printf("Error: The file '%s' is not a valid .img dump file. Adding '.img' extension automatically.\n", filename);
        strcat(filename, ".img"); // 自動補上 ".img"
    }

    save_filesystem(fs, filename);
    printf("Filesystem state saved to '%s'. Exiting.\n", filename);
}

